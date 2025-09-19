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
		AudioDeviceID device {kAudioObjectUnknown};
		std::function<void()> sampleRateChanged;
		std::function<void()> bufferSizeChanged;
	};

	CoreAudioDeviceStateChangeListener(const DeviceAndCallbacks &inDev, const DeviceAndCallbacks &outDev)
		: inDevice {inDev}
		, outDevice {outDev} {
		if (inDevice.device != kAudioObjectUnknown) {
			bindIn();
		}
		if (outDevice.device != kAudioObjectUnknown) {
			bindOut();
		}
	}

	~CoreAudioDeviceStateChangeListener() {
		if (inDevice.device != kAudioObjectUnknown) {
			unbindIn();
		}
		if (outDevice.device != kAudioObjectUnknown) {
			unbindOut();
		}
	}

	static OSStatus callback(AudioObjectID, UInt32, const AudioObjectPropertyAddress[], void *inClientData) {
		auto *fn = static_cast<std::function<void()> *>(inClientData);
		if (fn && *fn) {
			try {
				(*fn)();
			} catch (...) {}
		}
		return noErr;
	}

	void bindIn() {
		AudioObjectAddPropertyListener(
			inDevice.device, &sampleRateAddress, &callback, static_cast<void *>(&inSampleRateFn));
		AudioObjectAddPropertyListener(
			inDevice.device, &bufferSizeAddress, &callback, static_cast<void *>(&inBufferSizeFn));
	}

	void unbindIn() {
		AudioObjectRemovePropertyListener(
			inDevice.device, &sampleRateAddress, &callback, static_cast<void *>(&inSampleRateFn));
		AudioObjectRemovePropertyListener(
			inDevice.device, &bufferSizeAddress, &callback, static_cast<void *>(&inBufferSizeFn));
	}

	void bindOut() {
		AudioObjectAddPropertyListener(
			outDevice.device, &sampleRateAddress, &callback, static_cast<void *>(&outSampleRateFn));
		AudioObjectAddPropertyListener(
			outDevice.device, &bufferSizeAddress, &callback, static_cast<void *>(&outBufferSizeFn));
	}

	void unbindOut() {
		AudioObjectRemovePropertyListener(
			outDevice.device, &sampleRateAddress, &callback, static_cast<void *>(&outSampleRateFn));
		AudioObjectRemovePropertyListener(
			outDevice.device, &bufferSizeAddress, &callback, static_cast<void *>(&outBufferSizeFn));
	}

	AudioObjectPropertyAddress sampleRateAddress {
		kAudioDevicePropertyNominalSampleRate, kAudioObjectPropertyScopeGlobal, kAudioObjectPropertyElementMain};
	AudioObjectPropertyAddress bufferSizeAddress {
		kAudioDevicePropertyBufferFrameSize, kAudioObjectPropertyScopeGlobal, kAudioObjectPropertyElementMain};

	DeviceAndCallbacks inDevice;
	DeviceAndCallbacks outDevice;

	std::function<void()> &inSampleRateFn  = inDevice.sampleRateChanged;
	std::function<void()> &inBufferSizeFn  = inDevice.bufferSizeChanged;
	std::function<void()> &outSampleRateFn = outDevice.sampleRateChanged;
	std::function<void()> &outBufferSizeFn = outDevice.bufferSizeChanged;
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
	CoreAudioDeviceListener(const std::function<void()> &cb)
		: onDevicesChanged(cb) {
		context.fn = &onDevicesChanged;

		AudioObjectAddPropertyListener(kAudioObjectSystemObject, &deviceAddresses, &propProc, &context);
		AudioObjectAddPropertyListener(kAudioObjectSystemObject, &defaultInputAddress, &propProc, &context);
		AudioObjectAddPropertyListener(kAudioObjectSystemObject, &defaultOutputAddress, &propProc, &context);
	}

	~CoreAudioDeviceListener() {
		AudioObjectRemovePropertyListener(kAudioObjectSystemObject, &deviceAddresses, &propProc, &context);
		AudioObjectRemovePropertyListener(kAudioObjectSystemObject, &defaultInputAddress, &propProc, &context);
		AudioObjectRemovePropertyListener(kAudioObjectSystemObject, &defaultOutputAddress, &propProc, &context);
		context.fn = nullptr;
	}

private:
	struct Context {
		std::function<void()> *fn {nullptr};
	};

	static OSStatus propProc(AudioObjectID, UInt32, const AudioObjectPropertyAddress[], void *inClientData) {
		auto *context = static_cast<Context *>(inClientData);
		if (context && context->fn && *context->fn) {
			try {
				(*context->fn)();
			} catch (...) {}
		}
		return noErr;
	}

	AudioObjectPropertyAddress deviceAddresses {
		kAudioHardwarePropertyDevices, kAudioObjectPropertyScopeGlobal, kAudioObjectPropertyElementMain};
	AudioObjectPropertyAddress defaultInputAddress {kAudioHardwarePropertyDefaultInputDevice,
													kAudioObjectPropertyScopeGlobal,
													kAudioObjectPropertyElementMain};
	AudioObjectPropertyAddress defaultOutputAddress {kAudioHardwarePropertyDefaultOutputDevice,
													 kAudioObjectPropertyScopeGlobal,
													 kAudioObjectPropertyElementMain};

	std::function<void()> onDevicesChanged;
	Context context {};
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