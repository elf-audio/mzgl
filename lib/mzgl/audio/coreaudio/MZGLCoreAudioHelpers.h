#pragma once

#include <AudioToolbox/AudioToolbox.h>
#include <CoreAudio/CoreAudio.h>
#include <mach/mach_time.h>
#include <atomic>
#include <optional>
#include <vector>

struct InterleaveAudioBufferList {
	UInt32 mNumberBuffers;
	AudioBuffer mBuffers[1];
};

struct CoreAudioDeviceStateChangeListener {
	struct DeviceAndCallbacks {
		AudioDeviceID device;
		std::function<void()> sampleRateChanged;
		std::function<void()> bufferSizeChanged;
	};

	CoreAudioDeviceStateChangeListener(const DeviceAndCallbacks &_inDevice, const DeviceAndCallbacks &_outDevice)
		: inDevice {_inDevice}
		, outDevice {_outDevice} {
		if (inDevice.device != kAudioObjectUnknown) {
			bind(inDevice);
		}
		if (outDevice.device != kAudioObjectUnknown) {
			bind(outDevice);
		}
	}

	~CoreAudioDeviceStateChangeListener() {
		if (inDevice.device != kAudioObjectUnknown) {
			unbind(inDevice);
		}
		if (outDevice.device != kAudioObjectUnknown) {
			unbind(outDevice);
		}
	}

	void bind(DeviceAndCallbacks &device) {
		AudioObjectAddPropertyListenerBlock(
			device.device, &sampleRateAddress, deviceParamQueue, ^(UInt32, const AudioObjectPropertyAddress *) {
			  device.sampleRateChanged();
			});

		AudioObjectAddPropertyListenerBlock(
			device.device, &bufferSizeAddress, deviceParamQueue, ^(UInt32, const AudioObjectPropertyAddress *) {
			  device.bufferSizeChanged();
			});
	}

	void unbind(DeviceAndCallbacks &device) {
		AudioObjectRemovePropertyListenerBlock(device.device,
											   &sampleRateAddress,
											   deviceParamQueue,
											   reinterpret_cast<AudioObjectPropertyListenerBlock>(^ {}));
		AudioObjectRemovePropertyListenerBlock(device.device,
											   &bufferSizeAddress,
											   deviceParamQueue,
											   reinterpret_cast<AudioObjectPropertyListenerBlock>(^ {}));
	}

	AudioObjectPropertyAddress sampleRateAddress {
		kAudioDevicePropertyNominalSampleRate, kAudioObjectPropertyScopeGlobal, kAudioObjectPropertyElementMain};
	AudioObjectPropertyAddress bufferSizeAddress {
		kAudioDevicePropertyBufferFrameSize, kAudioObjectPropertyScopeGlobal, kAudioObjectPropertyElementMain};

	dispatch_queue_t deviceParamQueue =
		dispatch_queue_create("com.elfaudio.coreaudio.devparams", DISPATCH_QUEUE_SERIAL);
	DeviceAndCallbacks inDevice;
	DeviceAndCallbacks outDevice;
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

struct CoreAudioDeviceListener {
	CoreAudioDeviceListener(const std::function<void()> &_onDevicesChanged)
		: onDevicesChanged {_onDevicesChanged} {
		AudioObjectAddPropertyListenerBlock(kAudioObjectSystemObject,
											&deviceAddresses,
											deviceQueue,
											^(UInt32, const AudioObjectPropertyAddress *) {
											  if (onDevicesChanged) {
												  onDevicesChanged();
											  }
											});

		AudioObjectAddPropertyListenerBlock(kAudioObjectSystemObject,
											&defaultInputAddress,
											deviceQueue,
											^(UInt32, const AudioObjectPropertyAddress *) {
											  if (onDevicesChanged) {
												  onDevicesChanged();
											  }
											});

		AudioObjectAddPropertyListenerBlock(kAudioObjectSystemObject,
											&defaultOutputAddress,
											deviceQueue,
											^(UInt32, const AudioObjectPropertyAddress *) {
											  if (onDevicesChanged) {
												  onDevicesChanged();
											  }
											});
	}
	~CoreAudioDeviceListener() {
		if (!deviceQueue) {
			return;
		}
		AudioObjectRemovePropertyListenerBlock(kAudioObjectSystemObject,
											   &deviceAddresses,
											   deviceQueue,
											   reinterpret_cast<AudioObjectPropertyListenerBlock>(^ {}));
		AudioObjectRemovePropertyListenerBlock(kAudioObjectSystemObject,
											   &defaultInputAddress,
											   deviceQueue,
											   reinterpret_cast<AudioObjectPropertyListenerBlock>(^ {}));
		AudioObjectRemovePropertyListenerBlock(kAudioObjectSystemObject,
											   &defaultOutputAddress,
											   deviceQueue,
											   reinterpret_cast<AudioObjectPropertyListenerBlock>(^ {}));
		deviceQueue = nullptr;
	}

	AudioObjectPropertyAddress deviceAddresses {
		kAudioHardwarePropertyDevices, kAudioObjectPropertyScopeGlobal, kAudioObjectPropertyElementMain};
	AudioObjectPropertyAddress defaultInputAddress {kAudioHardwarePropertyDefaultInputDevice,
													kAudioObjectPropertyScopeGlobal,
													kAudioObjectPropertyElementMain};
	AudioObjectPropertyAddress defaultOutputAddress {kAudioHardwarePropertyDefaultOutputDevice,
													 kAudioObjectPropertyScopeGlobal,
													 kAudioObjectPropertyElementMain};

	dispatch_queue_t deviceQueue {dispatch_queue_create("com.elfaudio.coreaudio.devices", DISPATCH_QUEUE_SERIAL)};
	std::function<void()> onDevicesChanged;
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