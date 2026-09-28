#pragma once

#include <AudioToolbox/AudioToolbox.h>
#include <CoreAudio/CoreAudio.h>
#include <Block.h>
#include <dispatch/dispatch.h>
#include <mach/mach_time.h>
#include <atomic>
#include <functional>
#include <memory>
#include <optional>
#include <vector>

struct InterleaveAudioBufferList {
	UInt32 mNumberBuffers;
	AudioBuffer mBuffers[1];
};

/**
 * Listens for changes to one property of one CoreAudio object.
 *
 * The callback is delivered on the main queue, never on CoreAudio's own
 * listener thread, so it can safely touch (and restart) the audio system.
 * It is never called after this object is destroyed, and destroying it from
 * inside its own callback is fine. Create and destroy on the main thread.
 */
class CoreAudioPropertyListener {
public:
	CoreAudioPropertyListener(AudioObjectID objectId,
							  const AudioObjectPropertyAddress &propertyAddress,
							  std::function<void()> fn)
		: object {objectId}
		, address {propertyAddress}
		, callback {std::make_shared<std::function<void()>>(std::move(fn))} {
		std::weak_ptr<std::function<void()>> weakCallback = callback;
		block = Block_copy(^(UInt32, const AudioObjectPropertyAddress *) {
		  if (auto cb = weakCallback.lock(); cb && *cb) {
			  (*cb)();
		  }
		});
		AudioObjectAddPropertyListenerBlock(object, &address, dispatch_get_main_queue(), block);
	}

	~CoreAudioPropertyListener() {
		AudioObjectRemovePropertyListenerBlock(object, &address, dispatch_get_main_queue(), block);
		Block_release(block);
	}

	CoreAudioPropertyListener(const CoreAudioPropertyListener &)			= delete;
	CoreAudioPropertyListener &operator=(const CoreAudioPropertyListener &) = delete;

private:
	AudioObjectID object;
	AudioObjectPropertyAddress address;
	std::shared_ptr<std::function<void()>> callback;
	AudioObjectPropertyListenerBlock block {nullptr};
};

// Calls onChange when the sample rate or buffer size of any of the devices changes.
struct CoreAudioDeviceStateChangeListener {
	CoreAudioDeviceStateChangeListener(const std::vector<AudioDeviceID> &devices,
									   const std::function<void()> &onChange) {
		for (auto device: devices) {
			if (device == kAudioObjectUnknown) continue;
			listeners.push_back(std::make_unique<CoreAudioPropertyListener>(device, sampleRateAddress, onChange));
			listeners.push_back(std::make_unique<CoreAudioPropertyListener>(device, bufferSizeAddress, onChange));
		}
	}

private:
	static constexpr AudioObjectPropertyAddress sampleRateAddress {
		kAudioDevicePropertyNominalSampleRate, kAudioObjectPropertyScopeGlobal, kAudioObjectPropertyElementMain};
	static constexpr AudioObjectPropertyAddress bufferSizeAddress {
		kAudioDevicePropertyBufferFrameSize, kAudioObjectPropertyScopeGlobal, kAudioObjectPropertyElementMain};

	std::vector<std::unique_ptr<CoreAudioPropertyListener>> listeners;
};

struct CoreAudioState {
	~CoreAudioState() {
		if (audioUnitIn != nullptr) {
			AudioComponentInstanceDispose(audioUnitIn);
			audioUnitIn = nullptr;
		}

		if (audioUnitOut != nullptr) {
			AudioComponentInstanceDispose(audioUnitOut);
			audioUnitOut = nullptr;
		}
	}

	AudioUnit audioUnitIn {nullptr};
	AudioUnit audioUnitOut {nullptr};

	AudioDeviceID deviceIn {kAudioObjectUnknown};
	AudioDeviceID deviceOut {kAudioObjectUnknown};

	AudioStreamBasicDescription formatIn {};
	AudioStreamBasicDescription formatOut {};

	uint32_t bufferFrames = 512;
	double sampleRate	  = 48000.0;

	std::atomic<bool> running {false};
	std::atomic<bool> insideCallback {false};
	std::atomic<uint64_t> lastBufferBeginHostTime {0};

	std::atomic<int> inChans {0};
	std::atomic<int> outChans {0};

	std::vector<float> inputScratch;
	std::atomic<uint32_t> inputBufferCapacityFrames {0};

	std::unique_ptr<CoreAudioDeviceStateChangeListener> deviceListener;
};

// Calls onDevicesChanged when devices are added / removed or the default input / output changes.
struct CoreAudioDeviceListener {
	CoreAudioDeviceListener(const std::function<void()> &onDevicesChanged) {
		for (auto selector: {kAudioHardwarePropertyDevices,
							 kAudioHardwarePropertyDefaultInputDevice,
							 kAudioHardwarePropertyDefaultOutputDevice}) {
			listeners.push_back(std::make_unique<CoreAudioPropertyListener>(
				kAudioObjectSystemObject,
				AudioObjectPropertyAddress {
					selector, kAudioObjectPropertyScopeGlobal, kAudioObjectPropertyElementMain},
				onDevicesChanged));
		}
	}

private:
	std::vector<std::unique_ptr<CoreAudioPropertyListener>> listeners;
};

static inline uint64_t hostTimeToNanos(uint64_t hostTime) {
	static mach_timebase_info_data_t s_timebase;
	static std::atomic<bool> inited {false};
	if (!inited.load()) {
		(void) mach_timebase_info(&s_timebase);
		inited.store(true);
	}

	return (hostTime * s_timebase.numer) / s_timebase.denom;
}

static OSStatus inputRenderProc(void *inRefCon,
								AudioUnitRenderActionFlags *ioActionFlags,
								const AudioTimeStamp *inTimeStamp,
								UInt32,
								UInt32 inNumberFrames,
								AudioBufferList *) {
	if (auto *system = reinterpret_cast<CoreAudioSystem *>(inRefCon)) {
		if (!system->getState().audioUnitIn || system->getState().inChans <= 0) {
			return noErr;
		}

		const UInt32 framesToRender =
			std::min<UInt32>(inNumberFrames, system->getState().inputBufferCapacityFrames);

		InterleaveAudioBufferList abl {};
		abl.mNumberBuffers				= 1;
		abl.mBuffers[0].mNumberChannels = (UInt32) system->getState().inChans.load();
		abl.mBuffers[0].mDataByteSize =
			framesToRender * sizeof(float) * (UInt32) system->getState().inChans.load();
		abl.mBuffers[0].mData = system->getState().inputScratch.data();

		static constexpr UInt32 inputBusIndex = 1;

		auto result = AudioUnitRender(system->getState().audioUnitIn,
									  ioActionFlags,
									  inTimeStamp,
									  inputBusIndex,
									  framesToRender,
									  reinterpret_cast<AudioBufferList *>(&abl));
		if (result != noErr) {
			std::memset(system->getState().inputScratch.data(), 0, abl.mBuffers[0].mDataByteSize);

			system->inputCallback(
				system->getState().inputScratch.data(), (int) inNumberFrames, system->getState().inChans.load());
			return noErr;
		}

		if (framesToRender < inNumberFrames) {
			const size_t tailFrames	 = (size_t) inNumberFrames - framesToRender;
			const size_t tailSamples = tailFrames * (size_t) system->getState().inChans.load();
			std::memset(system->getState().inputScratch.data()
							+ (size_t) framesToRender * (size_t) system->getState().inChans.load(),
						0,
						tailSamples * sizeof(float));
		}

		system->inputCallback(
			system->getState().inputScratch.data(), (int) inNumberFrames, system->getState().inChans.load());
		return noErr;
	}
	return (OSStatus) -1;
}

static OSStatus outputRenderProc(void *inRefCon,
								 AudioUnitRenderActionFlags *,
								 const AudioTimeStamp *inTimeStamp,
								 UInt32,
								 UInt32 inNumberFrames,
								 AudioBufferList *ioData) {
	if (auto *system = reinterpret_cast<CoreAudioSystem *>(inRefCon)) {
		system->getState().insideCallback.store(true);
		system->getState().lastBufferBeginHostTime.store(inTimeStamp ? inTimeStamp->mHostTime
																	 : mach_absolute_time());
		if (system->getState().outChans > 0 && ioData && ioData->mNumberBuffers >= 1) {
			if (auto outputPtr = reinterpret_cast<float *>(ioData->mBuffers[0].mData)) {
				std::memset(outputPtr,
							0,
							inNumberFrames * sizeof(float)
								* static_cast<size_t>(system->getState().outChans.load()));
				system->outputCallback(
					outputPtr, static_cast<int>(inNumberFrames), system->getState().outChans.load());
			}
		}
		system->getState().insideCallback.store(false);
	}

	return noErr;
}