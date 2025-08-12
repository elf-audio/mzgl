#include "MZGLCoreAudio.h"

#include <AudioToolbox/AudioToolbox.h>
#include <CoreAudio/CoreAudio.h>
#include <mach/mach_time.h>
#include <atomic>
#include <vector>
#include <string>
#include <cstring>
#include <algorithm>

#include "log.h"

struct InterleaveAudioBufferList {
	UInt32 mNumberBuffers;
	AudioBuffer mBuffers[1];
};

struct CoreAudioState {
	AudioComponentInstance audioUnit {nullptr};
	AudioDeviceID deviceOut {kAudioObjectUnknown};
	AudioDeviceID deviceIn {kAudioObjectUnknown};
	AudioStreamBasicDescription fmt {};
	uint32_t bufferFrames = 512;
	double sampleRate	  = 48000.0;

	std::atomic<bool> running {false};
	std::atomic<bool> insideCallback {false};
	std::atomic<uint64_t> lastBufferBeginHostTime {0};

	std::atomic<int> inChans {0};
	std::atomic<int> outChans {0};

	std::vector<float> inputInterleaved;
	uint32_t inputBufferCapacityFrames = 0;
};

enum class DeviceType { Input, Output };

static AudioDeviceID getDefaultDevice(DeviceType type) {
	AudioObjectPropertyAddress address {type == DeviceType::Output ? kAudioHardwarePropertyDefaultOutputDevice
																   : kAudioHardwarePropertyDefaultInputDevice,
										kAudioObjectPropertyScopeGlobal,
										kAudioObjectPropertyElementMain};
	auto dev  = kAudioObjectUnknown;
	auto size = static_cast<UInt32>(sizeof(dev));

	if (AudioObjectGetPropertyData(kAudioObjectSystemObject, &address, 0, nullptr, &size, &dev) == noErr) {
		return dev;
	}
	return kAudioObjectUnknown;
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
	AudioObjectPropertyAddress address {kAudioDevicePropertyLatency,
										type == DeviceType::Output ? kAudioDevicePropertyScopeOutput
																   : kAudioDevicePropertyScopeInput,
										kAudioObjectPropertyElementMain};
	UInt32 latency = 0;
	auto size	   = static_cast<UInt32>(sizeof(latency));
	if (AudioObjectGetPropertyData(dev, &address, 0, nullptr, &size, &latency) != noErr) {
		return std::nullopt;
	}

	AudioObjectPropertyAddress safetyAddress {kAudioDevicePropertySafetyOffset,
											  type == DeviceType::Output ? kAudioDevicePropertyScopeOutput
																		 : kAudioDevicePropertyScopeInput,
											  kAudioObjectPropertyElementMain};
	UInt32 safety = 0;
	size		  = static_cast<UInt32>(sizeof(safety));
	if (AudioObjectGetPropertyData(dev, &safetyAddress, 0, nullptr, &size, &safety) == noErr) {
		latency += safety;
	}
	return latency;
}

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
						   UInt32 inBusNumber,
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

CoreAudioSystem::CoreAudioSystem()
	: state {std::make_unique<CoreAudioState>()} {
}

CoreAudioSystem::~CoreAudioSystem() {
	try {
		stop();
	} catch (...) {
		Log::e() << "Failed to stop CoreAudioSystem during destruction";
	}
}

double CoreAudioSystem::getPreferredSampleRate() const {
	auto outputSampleRate = getDeviceSampleRate(state->deviceOut);
	auto inputSampleRate  = getDeviceSampleRate(state->deviceIn);

	if (state->outChans > 0 && outputSampleRate.has_value()) {
		return *outputSampleRate;
	}

	if (state->inChans > 0 && inputSampleRate.has_value()) {
		return *inputSampleRate;
	}

	return defaultSampleRate;
}

uint32_t CoreAudioSystem::getPreferredNumberOfFrames() const {
	auto outputFrameSize = getDeviceFrameSize(state->deviceOut);
	if (outputFrameSize.has_value()) {
		return *outputFrameSize;
	}

	auto inputFrameSize = getDeviceFrameSize(state->deviceIn);
	if (inputFrameSize.has_value()) {
		return *inputFrameSize;
	}
	return defaultNumberOfFrames;
}

void CoreAudioSystem::setupState(int numInChannels, int numOutChannels) {
	state->inChans	= std::max(0, numInChannels);
	state->outChans = std::max(0, numOutChannels);

	state->deviceOut = getDefaultDevice(DeviceType::Output);
	state->deviceIn	 = getDefaultDevice(DeviceType::Input);

	state->sampleRate = getPreferredSampleRate();
	sampleRate		  = state->sampleRate;

	state->bufferFrames = getPreferredNumberOfFrames();
	bufferSize			= state->bufferFrames;
}

void CoreAudioSystem::createAudioUnit() {
	AudioComponentDescription description {
		.componentType		   = kAudioUnitType_Output,
		.componentSubType	   = kAudioUnitSubType_HALOutput,
		.componentManufacturer = kAudioUnitManufacturer_Apple,
		.componentFlags		   = 0,
		.componentFlagsMask	   = 0,
	};

	auto component = AudioComponentFindNext(nullptr, &description);
	if (component == nullptr) {
		throw std::runtime_error("CoreAudio: HAL Output component not found");
	}

	auto status = AudioComponentInstanceNew(component, &state->audioUnit);
	if (status != noErr) {
		throw std::runtime_error("CoreAudio: could not create AudioUnit with status " + std::to_string(status));
	}
}

void CoreAudioSystem::connectOutput() {
	if (state->deviceOut == kAudioObjectUnknown) {
		return;
	}

	auto result = AudioUnitSetProperty(state->audioUnit,
									   kAudioOutputUnitProperty_CurrentDevice,
									   kAudioUnitScope_Global,
									   0,
									   &state->deviceOut,
									   sizeof(state->deviceOut));

	if (result != noErr) {
		throw std::runtime_error("CoreAudio: failed to set output device with error " + std::to_string(result));
	}
}

void CoreAudioSystem::enableAudioIO() {
	UInt32 enableOutput = (state->outChans > 0) ? 1 : 0;
	AudioUnitSetProperty(state->audioUnit,
						 kAudioOutputUnitProperty_EnableIO,
						 kAudioUnitScope_Output,
						 0,
						 &enableOutput,
						 sizeof(enableOutput));

	UInt32 enableInput = (state->inChans > 0) ? 1 : 0;
	AudioUnitSetProperty(state->audioUnit,
						 kAudioOutputUnitProperty_EnableIO,
						 kAudioUnitScope_Input,
						 1,
						 &enableInput,
						 sizeof(enableInput));
}

void CoreAudioSystem::setupStreamFormat() {
	state->fmt				= {};
	state->fmt.mSampleRate	= state->sampleRate;
	state->fmt.mFormatID	= kAudioFormatLinearPCM;
	state->fmt.mFormatFlags = kAudioFormatFlagIsFloat | kAudioFormatFlagsNativeEndian | kAudioFormatFlagIsPacked;
	state->fmt.mBitsPerChannel	= sizeof(float) * 8;
	state->fmt.mFramesPerPacket = 1;

	if (state->outChans > 0) {
		AudioStreamBasicDescription outFmt = state->fmt;
		outFmt.mChannelsPerFrame		   = static_cast<UInt32>(state->outChans.load());
		outFmt.mBytesPerFrame			   = sizeof(float) * outFmt.mChannelsPerFrame;
		outFmt.mBytesPerPacket			   = outFmt.mBytesPerFrame * outFmt.mFramesPerPacket;

		auto result = AudioUnitSetProperty(
			state->audioUnit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input, 0, &outFmt, sizeof(outFmt));
		if (result != noErr) {
			throw std::runtime_error("CoreAudio: set output stream format failed with error "
									 + std::to_string(result));
		}
	}

	if (state->inChans > 0) {
		AudioStreamBasicDescription inFmt = state->fmt;
		inFmt.mChannelsPerFrame			  = static_cast<UInt32>(state->inChans.load());
		inFmt.mBytesPerFrame			  = sizeof(float) * inFmt.mChannelsPerFrame;
		inFmt.mBytesPerPacket			  = inFmt.mBytesPerFrame * inFmt.mFramesPerPacket;

		auto result = AudioUnitSetProperty(
			state->audioUnit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Output, 1, &inFmt, sizeof(inFmt));
		if (result != noErr) {
			throw std::runtime_error("CoreAudio: set input stream format failed with error "
									 + std::to_string(result));
		}
	}

	if (state->deviceOut != kAudioObjectUnknown) {
		auto frames = state->bufferFrames;
		AudioObjectPropertyAddress addr {
			kAudioDevicePropertyBufferFrameSize, kAudioDevicePropertyScopeOutput, kAudioObjectPropertyElementMain};
		UInt32 size = sizeof(frames);
		AudioObjectSetPropertyData(state->deviceOut, &addr, 0, nullptr, size, &frames);
	}

	UInt32 maxFrames = state->bufferFrames;
	AudioUnitSetProperty(state->audioUnit,
						 kAudioUnitProperty_MaximumFramesPerSlice,
						 kAudioUnitScope_Global,
						 0,
						 &maxFrames,
						 sizeof(maxFrames));
}

void CoreAudioSystem::setupAudioBuffers() {
	UInt32 maxFrames = state->bufferFrames;
	AudioUnitSetProperty(state->audioUnit,
						 kAudioUnitProperty_MaximumFramesPerSlice,
						 kAudioUnitScope_Global,
						 0,
						 &maxFrames,
						 sizeof(maxFrames));

	UInt32 size = sizeof(maxFrames);
	AudioUnitGetProperty(
		state->audioUnit, kAudioUnitProperty_MaximumFramesPerSlice, kAudioUnitScope_Global, 0, &maxFrames, &size);

	if (state->inChans > 0) {
		const size_t samples = static_cast<size_t>(maxFrames) * static_cast<size_t>(state->inChans);
		state->inputInterleaved.assign(samples, 0.0f);
		state->inputBufferCapacityFrames = maxFrames;
	}
}

void CoreAudioSystem::setupCallback() {
	AURenderCallbackStruct callback {};
	callback.inputProc		 = renderProc;
	callback.inputProcRefCon = this;
	auto result				 = AudioUnitSetProperty(state->audioUnit,
										kAudioUnitProperty_SetRenderCallback,
										kAudioUnitScope_Input,
										0,
										&callback,
										sizeof(callback));
	if (result != noErr) {
		throw std::runtime_error("CoreAudio: set render callback failed with error " + std::to_string(result));
	}

	result = AudioUnitInitialize(state->audioUnit);
	if (result != noErr) {
		throw std::runtime_error("CoreAudio: AudioUnitInitialize failed with error " + std::to_string(result));
	}
}

void CoreAudioSystem::setup(int numInChannels, int numOutChannels) {
	try {
		setupState(numInChannels, numOutChannels);
		createAudioUnit();
		connectOutput();
		enableAudioIO();
		setupStreamFormat();
		setupAudioBuffers();
		setupCallback();
	} catch (...) {
		Log::e() << "CoreAudioSystem setup failed";
		throw;
	}
}

void CoreAudioSystem::start() {
	if (state == nullptr) {
		Log::e() << "CoreAudioSystem state is null, cannot start";
		return;
	}

	if (state->audioUnit == nullptr) {
		Log::e() << "CoreAudioSystem audio unit is null, cannot start";
		return;
	}

	if (state->running.load()) {
		Log::e() << "CoreAudioSystem is already running, not starting again";
		return;
	}

	auto result = AudioOutputUnitStart(state->audioUnit);
	if (result != noErr) {
		Log::e() << "CoreAudioSystem failed to start audio unit with error " << result;
	}

	state->running.store(result == noErr);
}

void CoreAudioSystem::stop() {
	if (state == nullptr) {
		Log::e() << "CoreAudioSystem state is null, cannot stop";
		return;
	}

	if (state->audioUnit == nullptr) {
		Log::e() << "CoreAudioSystem audio unit is null, cannot stop";
		return;
	}

	if (!state->running.load()) {
		Log::e() << "CoreAudioSystem is not running, so cannot be stopped";
		return;
	}

	auto result = AudioOutputUnitStop(state->audioUnit);
	if (result != noErr) {
		Log::e() << "CoreAudioSystem failed to stop audio unit with error " << result;
	}
	state->running.store(result == noErr);
}

bool CoreAudioSystem::isRunning() {
	return state ? state->running.load() : false;
}

bool CoreAudioSystem::isInsideAudioCallback() {
	return state ? state->insideCallback.load() : false;
}

void CoreAudioSystem::setVerbose(bool _verbose) {
}

std::vector<AudioPort> CoreAudioSystem::getInputs() {
	return {};
}

std::vector<AudioPort> CoreAudioSystem::getOutputs() {
	return {};
}

bool CoreAudioSystem::setInput(const AudioPort &audioInput) {
}

bool CoreAudioSystem::setOutput(const AudioPort &audioOutput) {
}

void CoreAudioSystem::rescanPorts() {
}

AudioPort CoreAudioSystem::getInput() {
	return {};
}

AudioPort CoreAudioSystem::getOutput() {
	return {};
}

double CoreAudioSystem::getOutputLatency() {
	if (!state) {
		return 0.0;
	}

	auto frames = getDeviceLatency(state->deviceOut, DeviceType::Output);
	if (!frames.has_value()) {
		return 0.0;
	}

	return *frames / state->sampleRate;
}

double CoreAudioSystem::getNanoSecondsAtBufferBegin() {
	return static_cast<double>(hostTimeToNanos(state->lastBufferBeginHostTime.load()));
}

double CoreAudioSystem::getHostTime() {
	return static_cast<double>(hostTimeToNanos(AudioGetCurrentHostTime()));
}

CoreAudioState &CoreAudioSystem::getState() {
	return *state;
}