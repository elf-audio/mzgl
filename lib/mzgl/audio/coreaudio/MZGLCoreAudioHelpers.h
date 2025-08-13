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

struct AudioRing {
	std::vector<float> buf;
	size_t framesCap = 0;
	int chans		 = 0;
	std::atomic<size_t> r {0};
	std::atomic<size_t> w {0};

	void init(size_t capacityFrames, int channels) {
		chans	  = channels;
		framesCap = capacityFrames;
		buf.assign(framesCap * (size_t) chans, 0.0f);
		r = w = 0;
	}

	[[nodiscard]] size_t availableToRead() const {
		size_t rr = r.load(std::memory_order_acquire);
		size_t ww = w.load(std::memory_order_acquire);
		return (ww + framesCap - rr) % framesCap;
	}

	[[nodiscard]] size_t availableToWrite() const { return framesCap - 1 - availableToRead(); }

	[[nodiscard]] size_t write(const float *interleaved, size_t frames) {
		frames			= std::min(frames, availableToWrite());
		size_t ww		= w.load(std::memory_order_relaxed);
		size_t toEnd	= framesCap - ww;
		size_t f1		= std::min(frames, toEnd);
		size_t f2		= frames - f1;
		const size_t s1 = f1 * (size_t) chans;
		const size_t s2 = f2 * (size_t) chans;
		std::memcpy(&buf[ww * (size_t) chans], interleaved, s1 * sizeof(float));
		if (f2) std::memcpy(&buf[0], interleaved + s1, s2 * sizeof(float));
		w.store((ww + frames) % framesCap, std::memory_order_release);
		return frames;
	}

	[[nodiscard]] size_t read(float *interleavedOut, size_t frames) {
		frames			= std::min(frames, availableToRead());
		size_t rr		= r.load(std::memory_order_relaxed);
		size_t toEnd	= framesCap - rr;
		size_t f1		= std::min(frames, toEnd);
		size_t f2		= frames - f1;
		const size_t s1 = f1 * (size_t) chans;
		const size_t s2 = f2 * (size_t) chans;
		std::memcpy(interleavedOut, &buf[rr * (size_t) chans], s1 * sizeof(float));
		if (f2) std::memcpy(interleavedOut + s1, &buf[0], s2 * sizeof(float));
		r.store((rr + frames) % framesCap, std::memory_order_release);
		return frames;
	}
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
	AudioRing ring;
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
	if (state.audioUnitIn && state.inChans > 0) {
		InterleaveAudioBufferList abl;
		abl.mNumberBuffers				= 1;
		abl.mBuffers[0].mNumberChannels = (UInt32) state.inChans.load();
		abl.mBuffers[0].mDataByteSize	= inNumberFrames * sizeof(float) * (UInt32) state.inChans.load();
		abl.mBuffers[0].mData			= state.inputScratch.data();

		static constexpr UInt32 inputBusIndex = 1;

		auto result = AudioUnitRender(state.audioUnitIn,
									  ioActionFlags,
									  inTimeStamp,
									  inputBusIndex,
									  inNumberFrames,
									  reinterpret_cast<AudioBufferList *>(&abl));
		if (result != noErr) {
			std::memset(state.inputScratch.data(), 0, abl.mBuffers[0].mDataByteSize);
		} else {
			system.inputCallback(state.inputScratch.data(), (int) inNumberFrames, state.inChans.load());
			//
			//			(void) state.ring.write(state.inputScratch.data(), (size_t) inNumberFrames);
		}
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

static OSStatus inputRenderProc(void *inRefCon,
								AudioUnitRenderActionFlags *ioActionFlags,
								const AudioTimeStamp *inTimeStamp,
								UInt32 inBusNumber,
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

static OSStatus renderProc(void *inRefCon,
						   AudioUnitRenderActionFlags *ioActionFlags,
						   const AudioTimeStamp *inTimeStamp,
						   UInt32,
						   UInt32 inNumberFrames,
						   AudioBufferList *ioData) {
	if (auto *self = reinterpret_cast<CoreAudioSystem *>(inRefCon)) {
		startCallback(self->getState(), inTimeStamp);
		//		readInputs(*self, self->getState(), ioActionFlags, inTimeStamp, inNumberFrames);
		writeOutputs(*self, self->getState(), ioData, inNumberFrames);
		endCallback(self->getState());
	}

	return noErr;
}