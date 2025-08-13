#pragma once

#include <AudioToolbox/AudioToolbox.h>
#include <CoreAudio/CoreAudio.h>
#include <mach/mach_time.h>
#include <atomic>
#include <optional>
#include <vector>

#include "MZGLCoreAudioAggregrateHelpers.h"

struct InterleaveAudioBufferList {
	UInt32 mNumberBuffers;
	AudioBuffer mBuffers[1];
};

struct CoreAudioState {
	~CoreAudioState() {
		if (deviceOut != kAudioObjectUnknown) {
			destroyAggregateDevice(deviceOut);
			deviceOut = kAudioObjectUnknown;
		}
		if (audioUnit != nullptr) {
			AudioComponentInstanceDispose(audioUnit);
			audioUnit = nullptr;
		}
	}

	AudioComponentInstance audioUnit {nullptr};
	AudioDeviceID deviceOut {kAudioObjectUnknown};
	AudioDeviceID deviceIn {kAudioObjectUnknown};
	AudioStreamBasicDescription fmt {};

	std::atomic<bool> running {false};
	std::atomic<bool> insideCallback {false};
	std::atomic<uint64_t> lastBufferBeginHostTime {0};

	std::atomic<int> inChans {0};
	std::atomic<int> outChans {0};

	std::vector<float> inputInterleaved;
	uint32_t inputBufferCapacityFrames = 0;
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

static void readInputs(CoreAudioSystem &system,
					   CoreAudioState &state,
					   AudioUnitRenderActionFlags *ioActionFlags,
					   const AudioTimeStamp *inTimeStamp,
					   UInt32 inNumberFrames) {
	if (state.inChans <= 0) {
		return;
	}

	const auto framesToRender = std::min<UInt32>(inNumberFrames, state.inputBufferCapacityFrames);

	InterleaveAudioBufferList audioBufferList {};
	audioBufferList.mNumberBuffers				= 1;
	audioBufferList.mBuffers[0].mNumberChannels = static_cast<UInt32>(state.inChans.load());
	audioBufferList.mBuffers[0].mDataByteSize =
		framesToRender * sizeof(float) * audioBufferList.mBuffers[0].mNumberChannels;
	audioBufferList.mBuffers[0].mData = state.inputInterleaved.data();

	static constexpr UInt32 inputBusIndex = 1;

	auto result = AudioUnitRender(state.audioUnit,
								  ioActionFlags,
								  inTimeStamp,
								  inputBusIndex,
								  framesToRender,
								  reinterpret_cast<AudioBufferList *>(&audioBufferList));

	float *inputPtr = (result == noErr) ? state.inputInterleaved.data() : nullptr;

	if (result == noErr && framesToRender < inNumberFrames && inputPtr) {
		auto tailFrames	 = inNumberFrames - framesToRender;
		auto tailSamples = tailFrames * static_cast<size_t>(state.inChans.load());
		std::memset(inputPtr + framesToRender * state.inChans.load(), 0, tailSamples * sizeof(float));
	}

	if (inputPtr) {
		system.inputCallback(inputPtr, static_cast<int>(inNumberFrames), state.inChans.load());
	}
}

static void
	writeOutputs(CoreAudioSystem &system, CoreAudioState &state, AudioBufferList *ioData, UInt32 inNumberFrames) {
	if (state.outChans > 0 && ioData && ioData->mNumberBuffers >= 1) {
		if (auto outputPtr = reinterpret_cast<float *>(ioData->mBuffers[0].mData)) {
			std::memset(outputPtr, 0, inNumberFrames * sizeof(float) * static_cast<size_t>(state.outChans.load()));
			system.outputCallback(outputPtr, static_cast<int>(inNumberFrames), state.outChans.load());
		}
	}
}

static void startCallback(CoreAudioState &state, const AudioTimeStamp *inTimeStamp) {
	state.insideCallback.store(true);
	state.lastBufferBeginHostTime.store(inTimeStamp ? inTimeStamp->mHostTime : mach_absolute_time());
}

static void endCallback(CoreAudioState &state) {
	state.insideCallback.store(false);
}

static OSStatus renderProc(void *inRefCon,
						   AudioUnitRenderActionFlags *ioActionFlags,
						   const AudioTimeStamp *inTimeStamp,
						   UInt32,
						   UInt32 inNumberFrames,
						   AudioBufferList *ioData) {
	if (auto *self = reinterpret_cast<CoreAudioSystem *>(inRefCon)) {
		startCallback(self->getState(), inTimeStamp);
		readInputs(*self, self->getState(), ioActionFlags, inTimeStamp, inNumberFrames);
		writeOutputs(*self, self->getState(), ioData, inNumberFrames);
		endCallback(self->getState());
	}

	return noErr;
}