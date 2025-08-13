#pragma once

#include <AudioToolbox/AudioToolbox.h>
#include <CoreAudio/CoreAudio.h>
#include <mach/mach_time.h>
#include <atomic>
#include <optional>
#include <vector>

struct CFReleaseGuard {
	CFReleaseGuard() = default;
	CFReleaseGuard(CFTypeRef _obj)
		: obj(_obj) {}
	~CFReleaseGuard() {
		if (obj) {
			CFRelease(obj);
		}
	}
	operator CFTypeRef() const { return obj; }
	CFTypeRef *operator&() { return &obj; }
	CFStringRef asStringRef() const { return (CFStringRef) obj; }
	CFTypeRef obj {nullptr};
};

static std::optional<CFStringRef> getDeviceUIDCF(AudioDeviceID dev) {
	AudioObjectPropertyAddress address {
		kAudioDevicePropertyDeviceUID, kAudioObjectPropertyScopeGlobal, kAudioObjectPropertyElementMain};
	CFStringRef uid = nullptr;
	auto size		= static_cast<UInt32>(sizeof(uid));
	if (AudioObjectGetPropertyData(dev, &address, 0, nullptr, &size, &uid) != noErr || uid == nullptr) {
		return std::nullopt;
	}
	return uid;
}

static std::optional<std::string> getDeviceUID(AudioDeviceID dev) {
	auto maybe = getDeviceUIDCF(dev);
	if (!maybe) {
		return std::nullopt;
	}

	CFReleaseGuard uidRG {*maybe};

	char buf[1024];
	if (CFStringGetCString((CFStringRef) uidRG.obj, buf, sizeof(buf), kCFStringEncodingUTF8)) {
		return std::string(buf);
	}
	return std::nullopt;
}

static std::optional<AudioDeviceID> createAggregateDevice(AudioDeviceID inputDev,
														  AudioDeviceID outputDev,
														  const std::string &aggregateName,
														  bool outAsClockMaster			  = true,
														  bool enableDriftCompOnNonMaster = true) {
	auto inUIDStr  = getDeviceUID(inputDev);
	auto outUIDStr = getDeviceUID(outputDev);
	if (!inUIDStr || !outUIDStr) {
		return std::nullopt;
	}

	auto aggUIDStr = std::string("com.mzgl.aggregate.") + *outUIDStr + "-" + *inUIDStr;

	CFReleaseGuard inUIDCF {CFStringCreateWithCString(nullptr, inUIDStr->c_str(), kCFStringEncodingUTF8)};
	CFReleaseGuard outUIDCF {CFStringCreateWithCString(nullptr, outUIDStr->c_str(), kCFStringEncodingUTF8)};
	CFReleaseGuard aggNameCF {CFStringCreateWithCString(nullptr, aggUIDStr.c_str(), kCFStringEncodingUTF8)};
	CFReleaseGuard aggUIDCF {CFStringCreateWithCString(nullptr, aggregateName.c_str(), kCFStringEncodingUTF8)};

	auto masterUIDCF = outAsClockMaster ? outUIDCF.asStringRef() : inUIDCF.asStringRef();

	auto makeSubDict = [](CFStringRef uid, bool driftComp) -> CFMutableDictionaryRef {
		CFMutableDictionaryRef d = CFDictionaryCreateMutable(
			nullptr, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
		CFDictionarySetValue(d, CFSTR(kAudioSubDeviceUIDKey), uid);

		if (driftComp) {
			CFDictionarySetValue(d, CFSTR(kAudioSubDeviceDriftCompensationKey), kCFBooleanTrue);
		}
		return d;
	};

	auto subIn			  = makeSubDict(inUIDCF.asStringRef(), enableDriftCompOnNonMaster && !outAsClockMaster);
	auto subOut			  = makeSubDict(outUIDCF.asStringRef(), enableDriftCompOnNonMaster && outAsClockMaster);
	const void *values[2] = {subOut, subIn};
	CFReleaseGuard subListCF {CFArrayCreate(nullptr, values, 2, &kCFTypeArrayCallBacks)};

	CFRelease(subIn);
	CFRelease(subOut);

	CFMutableDictionaryRef aggDict =
		CFDictionaryCreateMutable(nullptr, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
	CFReleaseGuard dictCF {aggDict};

	CFDictionarySetValue(aggDict, CFSTR(kAudioAggregateDeviceNameKey), aggNameCF.obj);
	CFDictionarySetValue(aggDict, CFSTR(kAudioAggregateDeviceUIDKey), aggUIDCF.obj);
	CFDictionarySetValue(aggDict, CFSTR(kAudioAggregateDeviceSubDeviceListKey), subListCF.obj);
	CFDictionarySetValue(aggDict, CFSTR(kAudioAggregateDeviceMasterSubDeviceKey), masterUIDCF);
	CFDictionarySetValue(aggDict, CFSTR(kAudioAggregateDeviceIsPrivateKey), kCFBooleanTrue);

	AudioDeviceID newDev = kAudioObjectUnknown;
	OSStatus result		 = AudioHardwareCreateAggregateDevice(aggDict, &newDev);
	if (result != noErr || newDev == kAudioObjectUnknown) {
		return std::nullopt;
	}

	return newDev;
}

static bool destroyAggregateDevice(AudioDeviceID aggregateDev) {
	if (aggregateDev == kAudioObjectUnknown) {
		return false;
	}
	return AudioHardwareDestroyAggregateDevice(aggregateDev) == noErr;
}