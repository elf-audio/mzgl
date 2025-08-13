#pragma once

#include <AudioToolbox/AudioToolbox.h>
#include <CoreAudio/CoreAudio.h>
#include <mach/mach_time.h>
#include <atomic>
#include <optional>
#include <vector>
#include <set>

#include "AudioSystem.h"
#include "log.h"

enum class DeviceType { Input, Output };

static AudioObjectPropertyScope deviceTypeToCoreAudioScope(DeviceType type) {
	return type == DeviceType::Output ? kAudioObjectPropertyScopeOutput : kAudioObjectPropertyScopeInput;
}

static auto deviceTypeToCoreAudioHardwareProperty(DeviceType type) {
	return type == DeviceType::Output ? kAudioHardwarePropertyDefaultOutputDevice
									  : kAudioHardwarePropertyDefaultInputDevice;
}

static std::string convertCFStringToStdString(CFStringRef string) {
	if (string == nullptr) {
		return {};
	}

	char buf[1024];
	if (CFStringGetCString(string, buf, sizeof(buf), kCFStringEncodingUTF8)) {
		return std::string(buf);
	}

	CFIndex len = CFStringGetLength(string);
	CFIndex max = CFStringGetMaximumSizeForEncoding(len, kCFStringEncodingUTF8) + 1;
	std::string out;
	out.resize((size_t) max);
	if (CFStringGetCString(string, out.data(), max, kCFStringEncodingUTF8)) {
		out.resize(std::char_traits<char>::length(out.c_str()));
		return out;
	}
	return {};
}

static std::optional<std::string> getDeviceName(AudioDeviceID dev) {
	if (dev == kAudioObjectUnknown) {
		return std::nullopt;
	}

	AudioObjectPropertyAddress address {
		kAudioObjectPropertyName, kAudioObjectPropertyScopeGlobal, kAudioObjectPropertyElementMain};
	CFStringRef cfName = nullptr;
	auto size		   = static_cast<UInt32>(sizeof(cfName));
	if (AudioObjectGetPropertyData(dev, &address, 0, nullptr, &size, &cfName) == noErr && cfName) {
		std::string name = convertCFStringToStdString(cfName);
		CFRelease(cfName);
		return name;
	}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
	address.mSelector = kAudioDevicePropertyDeviceNameCFString;
#pragma clang diagnostic pop

	size = sizeof(cfName);
	if (AudioObjectGetPropertyData(dev, &address, 0, nullptr, &size, &cfName) == noErr && cfName) {
		std::string name = convertCFStringToStdString(cfName);
		CFRelease(cfName);
		return name;
	}

	return std::nullopt;
}

static std::optional<double> getDeviceSampleRate(AudioDeviceID dev) {
	if (dev == kAudioObjectUnknown) {
		return std::nullopt;
	}

	AudioObjectPropertyAddress address {
		kAudioDevicePropertyNominalSampleRate, kAudioObjectPropertyScopeGlobal, kAudioObjectPropertyElementMain};
	Float64 sampleRate = 0.0;
	UInt32 size		   = static_cast<UInt32>(sizeof(sampleRate));
	if (AudioObjectGetPropertyData(dev, &address, 0, nullptr, &size, &sampleRate) == noErr) {
		return static_cast<double>(sampleRate);
	}

	return std::nullopt;
}

static std::vector<double> getDeviceSupportedSampleRates(AudioDeviceID dev) {
	if (dev == kAudioObjectUnknown) {
		return {};
	}

	AudioObjectPropertyAddress address {kAudioDevicePropertyAvailableNominalSampleRates,
										kAudioObjectPropertyScopeGlobal,
										kAudioObjectPropertyElementMain};

	auto size = static_cast<UInt32>(0);
	if (AudioObjectGetPropertyDataSize(dev, &address, 0, nullptr, &size) != noErr || size == 0) {
		return {};
	}

	const UInt32 count = size / sizeof(AudioValueRange);
	std::vector<AudioValueRange> ranges(count);
	if (AudioObjectGetPropertyData(dev, &address, 0, nullptr, &size, ranges.data()) != noErr) {
		return {};
	}

	std::set<double> sampleRateSet;
	for (const auto &r: ranges) {
		sampleRateSet.insert(r.mMinimum);
		if (r.mMaximum != r.mMinimum) {
			sampleRateSet.insert(r.mMaximum);
		}
	}

	std::vector<double> sampleRates(sampleRateSet.begin(), sampleRateSet.end());
	return sampleRates;
}

static std::optional<UInt32> getDeviceFrameSize(AudioDeviceID dev) {
	if (dev == kAudioObjectUnknown) {
		return std::nullopt;
	}

	AudioObjectPropertyAddress address {
		kAudioDevicePropertyBufferFrameSize, kAudioObjectPropertyScopeGlobal, kAudioObjectPropertyElementMain};
	UInt32 frames = 0;
	auto size	  = static_cast<UInt32>(sizeof(frames));

	if (AudioObjectGetPropertyData(dev, &address, 0, nullptr, &size, &frames) == noErr) {
		return frames;
	}
	return std::nullopt;
}

static std::optional<UInt32> getDeviceLatency(AudioDeviceID dev, DeviceType type) {
	if (dev == kAudioObjectUnknown) {
		return std::nullopt;
	}

	AudioObjectPropertyAddress address {
		kAudioDevicePropertyLatency, deviceTypeToCoreAudioScope(type), kAudioObjectPropertyElementMain};

	UInt32 latency = 0;
	auto size	   = static_cast<UInt32>(sizeof(latency));
	if (AudioObjectGetPropertyData(dev, &address, 0, nullptr, &size, &latency) != noErr) {
		return std::nullopt;
	}

	AudioObjectPropertyAddress safetyAddress {
		kAudioDevicePropertySafetyOffset, deviceTypeToCoreAudioScope(type), kAudioObjectPropertyElementMain};
	UInt32 safety = 0;
	size		  = static_cast<UInt32>(sizeof(safety));
	if (AudioObjectGetPropertyData(dev, &safetyAddress, 0, nullptr, &size, &safety) == noErr) {
		latency += safety;
	}
	return latency;
}

static std::optional<UInt32> getDeviceChannelCount(AudioDeviceID dev, DeviceType type) {
	if (dev == kAudioObjectUnknown) {
		return std::nullopt;
	}

	AudioObjectPropertyAddress address {kAudioDevicePropertyStreamConfiguration,
										deviceTypeToCoreAudioScope(type),
										kAudioObjectPropertyElementMain};

	auto size = static_cast<UInt32>(0);
	if (AudioObjectGetPropertyDataSize(dev, &address, 0, nullptr, &size) != noErr || size == 0) {
		return std::nullopt;
	}

	std::vector<uint8_t> storage(size);
	auto *abl = reinterpret_cast<AudioBufferList *>(storage.data());
	if (AudioObjectGetPropertyData(dev, &address, 0, nullptr, &size, abl) != noErr) {
		return std::nullopt;
	}

	UInt32 channels = 0;
	for (UInt32 i = 0; i < abl->mNumberBuffers; ++i) {
		channels += abl->mBuffers[i].mNumberChannels;
	}
	return channels;
}

static AudioDeviceID getDefaultDevice(DeviceType type) {
	AudioObjectPropertyAddress address {deviceTypeToCoreAudioHardwareProperty(type),
										kAudioObjectPropertyScopeGlobal,
										kAudioObjectPropertyElementMain};
	auto dev  = kAudioObjectUnknown;
	auto size = static_cast<UInt32>(sizeof(dev));

	if (AudioObjectGetPropertyData(kAudioObjectSystemObject, &address, 0, nullptr, &size, &dev) == noErr) {
		return dev;
	}
	return kAudioObjectUnknown;
}

static std::vector<AudioDeviceID> getAllDevices() {
	AudioObjectPropertyAddress address {
		kAudioHardwarePropertyDevices, kAudioObjectPropertyScopeGlobal, kAudioObjectPropertyElementMain};
	auto size = static_cast<UInt32>(0);
	if (AudioObjectGetPropertyDataSize(kAudioObjectSystemObject, &address, 0, nullptr, &size) != noErr
		|| size == 0) {
		return {};
	}

	UInt32 deviceCount = size / sizeof(AudioDeviceID);
	std::vector<AudioDeviceID> devices(deviceCount);
	if (AudioObjectGetPropertyData(kAudioObjectSystemObject, &address, 0, nullptr, &size, devices.data())
		!= noErr) {
		return {};
	}

	return devices;
}

static void getDeviceList(std::vector<AudioPort> &inputPorts, std::vector<AudioPort> &outputPorts) {
	inputPorts.clear();
	outputPorts.clear();

	auto devices	= getAllDevices();
	auto defaultIn	= getDefaultDevice(DeviceType::Input);
	auto defaultOut = getDefaultDevice(DeviceType::Output);

	for (auto device: devices) {
		auto name = getDeviceName(device);
		if (!name.has_value()) {
			Log::d() << "Skipping device with no name: " << std::to_string(device);
			continue;
		}

		auto inChannels	 = getDeviceChannelCount(device, DeviceType::Input);
		auto outChannels = getDeviceChannelCount(device, DeviceType::Output);

		if (inChannels == 0 && outChannels == 0) {
			Log::d() << "Skipping device with no channels: " << *name;
			continue;
		}

		auto sampleRate = getDeviceSampleRate(device);
		if (!sampleRate.has_value()) {
			Log::d() << "Skipping device with no sample rate: " << *name;
			continue;
		}

		auto supportedSampleRates = getDeviceSupportedSampleRates(device);
		if (supportedSampleRates.empty()) {
			Log::d() << "Skipping device with no supported sample rates: " << *name;
			continue;
		}

		if (inChannels > 0) {
			inputPorts.push_back({
				.portId				  = static_cast<int>(device),
				.numInChannels		  = static_cast<int>(*inChannels),
				.numOutChannels		  = 0,
				.defaultSampleRate	  = *sampleRate,
				.supportedSampleRates = supportedSampleRates,
				.name				  = *name,
				.isDefaultInput		  = device == defaultIn,
				.isDefaultOutput	  = false,
			});
		}

		if (outChannels > 0) {
			outputPorts.push_back({
				.portId				  = static_cast<int>(device),
				.numInChannels		  = 0,
				.numOutChannels		  = static_cast<int>(*outChannels),
				.defaultSampleRate	  = *sampleRate,
				.supportedSampleRates = supportedSampleRates,
				.name				  = *name,
				.isDefaultInput		  = false,
				.isDefaultOutput	  = device == defaultOut,
			});
		}
	}
}