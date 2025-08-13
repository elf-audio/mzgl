#include "MZGLCoreAudio.h"

#include <atomic>
#include <vector>
#include <string>
#include <cstring>
#include <algorithm>

#include "MZGLCoreAudioDeviceHelpers.h"
#include "MZGLCoreAudioHelpers.h"
#include "log.h"
#include "mzAssert.h"

CoreAudioSystem::CoreAudioSystem() {
#ifdef DEBUG
	verbose = true;
#endif
	rescanPorts();
}

CoreAudioSystem::~CoreAudioSystem() {
	try {
		stop();
	} catch (...) {
		Log::e() << "Failed to stop CoreAudioSystem during destruction";
	}
}

std::optional<double> getSampleRateForPort(const std::optional<AudioPort> &port) {
	if (port.has_value() && (port->numOutChannels > 0 || port->numInChannels > 0)) {
		if (auto sampleRate = getDeviceSampleRate(port->portId); sampleRate.has_value()) {
			return *sampleRate;
		}
	}
	return std::nullopt;
}

std::optional<uint32_t> getFrameSizeForPort(const std::optional<AudioPort> &port) {
	if (port.has_value() && (port->numOutChannels > 0 || port->numInChannels > 0)) {
		if (auto frameSize = getDeviceFrameSize(port->portId); frameSize.has_value()) {
			return *frameSize;
		}
	}
	return std::nullopt;
}

double CoreAudioSystem::getPreferredSampleRate() const {
	if (auto sr = getSampleRateForPort(outputPort); sr.has_value()) {
		return *sr;
	}

	if (auto sr = getSampleRateForPort(inputPort); sr.has_value()) {
		return *sr;
	}

	return defaultSampleRate;
}

uint32_t CoreAudioSystem::getPreferredNumberOfFrames() const {
	if (auto frameSize = getFrameSizeForPort(outputPort); frameSize.has_value()) {
		return *frameSize;
	}

	if (auto frameSize = getFrameSizeForPort(inputPort); frameSize.has_value()) {
		return *frameSize;
	}

	return defaultNumberOfFrames;
}

void CoreAudioSystem::setupState(int numInChannels, int numOutChannels) {
	if (!inputPort.has_value() || !outputPort.has_value()) {
		rescanPorts();
	}

	if (verbose) {
		Log::d() << "CORE AUDIO STARTUP:";
		Log::d() << "  Input Port: " << (inputPort.has_value() ? inputPort->name : "none");
		Log::d() << "  Output Port: " << (outputPort.has_value() ? outputPort->name : "none");
		Log::d() << "  Sample Rate: " << sampleRate;
		Log::d() << "  Buffer Size: " << bufferSize;
		Log::d() << "  In Channels: " << numInChannels;
		Log::d() << "  Out Channels: " << numOutChannels;
	}

	mzAssert(inputPort.has_value() && outputPort.has_value());

	state				= std::make_unique<CoreAudioState>();
	state->inChans		= std::max(0, numInChannels);
	state->outChans		= std::max(0, numOutChannels);
	state->deviceOut	= outputPort.has_value() ? outputPort->portId : kAudioObjectUnknown;
	state->deviceIn		= inputPort.has_value() ? inputPort->portId : kAudioObjectUnknown;
	state->sampleRate	= sampleRate;
	state->bufferFrames = bufferSize;

	setDeviceSampleRate(state->deviceOut, sampleRate);
	setDeviceSampleRate(state->deviceIn, sampleRate);
	setDeviceBufferFrames(state->deviceOut, kAudioObjectPropertyScopeOutput, (UInt32) bufferSize);
	setDeviceBufferFrames(state->deviceIn, kAudioObjectPropertyScopeInput, (UInt32) bufferSize);

	state->formatIn	 = makeInterleavedASBD(sampleRate, state->inChans);
	state->formatOut = makeInterleavedASBD(sampleRate, state->outChans);

	//	state->inputScratch.assign((size_t) bufferSize * (size_t) std::max(1, state->inChans.load()), 0.0f);
	//	state->ring.init((size_t) bufferSize * 8, std::max(1, state->inChans.load()));
}

void CoreAudioSystem::createInputAudioUnit() {
	AudioComponentDescription description {.componentType		  = kAudioUnitType_Output,
										   .componentSubType	  = kAudioUnitSubType_HALOutput,
										   .componentManufacturer = kAudioUnitManufacturer_Apple};
	auto component = AudioComponentFindNext(nullptr, &description);
	if (component == nullptr) {
		throw std::runtime_error("CoreAudio: failed to find input audio component");
	}

	auto result = AudioComponentInstanceNew(component, &state->audioUnitIn);
	if (result != noErr) {
		mzAssert(result == noErr);
		throw std::runtime_error("CoreAudio: failed to create input audio unit with error "
								 + std::to_string(result));
	}

	UInt32 zero = 0;
	UInt32 one	= 1;

	result = AudioUnitSetProperty(
		state->audioUnitIn, kAudioOutputUnitProperty_EnableIO, kAudioUnitScope_Input, 1, &one, sizeof(one));
	if (result != noErr) {
		mzAssert(result == noErr);
		throw std::runtime_error("CoreAudio: failed to set input scope enable " + std::to_string(result));
	}

	result = AudioUnitSetProperty(
		state->audioUnitIn, kAudioOutputUnitProperty_EnableIO, kAudioUnitScope_Output, 0, &zero, sizeof(zero));
	if (result != noErr) {
		mzAssert(result == noErr);
		throw std::runtime_error("CoreAudio: failed to set output scope enable " + std::to_string(result));
	}

	if (state->deviceIn != kAudioObjectUnknown) {
		result = AudioUnitSetProperty(state->audioUnitIn,
									  kAudioOutputUnitProperty_CurrentDevice,
									  kAudioUnitScope_Global,
									  0,
									  &state->deviceIn,
									  sizeof(state->deviceIn));
		if (result != noErr) {
			mzAssert(result == noErr);
			throw std::runtime_error("CoreAudio: failed to set current device " + std::to_string(result));
		}
	}

	result = AudioUnitSetProperty(state->audioUnitIn,
								  kAudioUnitProperty_StreamFormat,
								  kAudioUnitScope_Output,
								  1,
								  &state->formatIn,
								  sizeof(state->formatIn));
	if (result != noErr) {
		mzAssert(result == noErr);
		throw std::runtime_error("CoreAudio: failed to set stream format " + std::to_string(result));
	}

	setMaxFramesPerSlice(state->audioUnitIn, (UInt32) state->bufferFrames);

	result = AudioUnitInitialize(state->audioUnitIn);
	if (result != noErr) {
		mzAssert(result == noErr);
		throw std::runtime_error("CoreAudio: failed to initialize input au " + std::to_string(result));
	}

	computeAndAllocInputCapacity();

	AURenderCallbackStruct callback {};
	callback.inputProc		 = inputRenderProc;
	callback.inputProcRefCon = this;

	result = AudioUnitSetProperty(state->audioUnitIn,
								  kAudioOutputUnitProperty_SetInputCallback,
								  kAudioUnitScope_Global,
								  0,
								  &callback,
								  sizeof(callback));
	//	result = AudioUnitAddRenderNotify(state->audioUnitIn, inputRenderProc, this);
	if (result != noErr) {
		mzAssert(result == noErr);
		throw std::runtime_error("CoreAudio: failed to set render notify " + std::to_string(result));
	}
}

void CoreAudioSystem::computeAndAllocInputCapacity() {
	UInt32 auMaxSlice = getDeviceMaxFrameSize(state->audioUnitIn);
	if (auMaxSlice == 0) {
		auMaxSlice = (UInt32) bufferSize;
	}

	UInt32 capacityFrames = std::max({auMaxSlice,
									  getDeviceFrameSize(state->deviceIn).value_or(bufferSize),
									  static_cast<UInt32>(state->bufferFrames)});

	state->inputBufferCapacityFrames = capacityFrames;
	const size_t samples			 = (size_t) capacityFrames * (size_t) std::max(1, state->inChans.load());
	state->inputScratch.assign(samples, 0.0f);

	state->ring.init((size_t) capacityFrames * 8, std::max(1, state->inChans.load()));
}

void CoreAudioSystem::createOutputAudioUnit() {
	AudioComponentDescription description {.componentType		  = kAudioUnitType_Output,
										   .componentSubType	  = kAudioUnitSubType_HALOutput,
										   .componentManufacturer = kAudioUnitManufacturer_Apple};
	auto component = AudioComponentFindNext(nullptr, &description);
	if (component == nullptr) {
		throw std::runtime_error("CoreAudio: failed to find input audio component");
	}

	auto result = AudioComponentInstanceNew(component, &state->audioUnitOut);
	if (result != noErr) {
		mzAssert(result == noErr);
		throw std::runtime_error("CoreAudio: failed to create output audio unit with error "
								 + std::to_string(result));
	}

	UInt32 zero = 0;
	UInt32 one	= 1;

	result = AudioUnitSetProperty(
		state->audioUnitOut, kAudioOutputUnitProperty_EnableIO, kAudioUnitScope_Output, 0, &one, sizeof(one));
	if (result != noErr) {
		mzAssert(result == noErr);
		throw std::runtime_error("CoreAudio: failed to set output scope enable " + std::to_string(result));
	}

	result = AudioUnitSetProperty(
		state->audioUnitOut, kAudioOutputUnitProperty_EnableIO, kAudioUnitScope_Input, 1, &zero, sizeof(zero));
	if (result != noErr) {
		mzAssert(result == noErr);
		throw std::runtime_error("CoreAudio: failed to set input scope enable " + std::to_string(result));
	}

	if (state->deviceOut != kAudioObjectUnknown) {
		result = AudioUnitSetProperty(state->audioUnitOut,
									  kAudioOutputUnitProperty_CurrentDevice,
									  kAudioUnitScope_Global,
									  0,
									  &state->deviceOut,
									  sizeof(state->deviceOut));
		if (result != noErr) {
			mzAssert(result == noErr);
			throw std::runtime_error("CoreAudio: failed to set output current device " + std::to_string(result));
		}
	}

	result = AudioUnitSetProperty(state->audioUnitOut,
								  kAudioUnitProperty_StreamFormat,
								  kAudioUnitScope_Input,
								  0,
								  &state->formatOut,
								  sizeof(state->formatOut));
	if (result != noErr) {
		mzAssert(result == noErr);
		throw std::runtime_error("CoreAudio: failed to set stream format " + std::to_string(result));
	}

	setMaxFramesPerSlice(state->audioUnitOut, (UInt32) state->bufferFrames);

	AURenderCallbackStruct callback {};
	callback.inputProc		 = renderProc;
	callback.inputProcRefCon = this;
	result					 = AudioUnitSetProperty(state->audioUnitOut,
									kAudioUnitProperty_SetRenderCallback,
									kAudioUnitScope_Input,
									0,
									&callback,
									sizeof(callback));
	if (result != noErr) {
		mzAssert(result == noErr);
		throw std::runtime_error("CoreAudio: failed to set callback " + std::to_string(result));
	}

	result = AudioUnitInitialize(state->audioUnitOut);
	if (result != noErr) {
		mzAssert(result == noErr);
		throw std::runtime_error("CoreAudio: failed to start output au " + std::to_string(result));
	}
}

//void CoreAudioSystem::connectOutput() {
//	if (state->deviceOut == kAudioObjectUnknown) {
//		return;
//	}
//
//	auto result = AudioUnitSetProperty(state->audioUnit,
//									   kAudioOutputUnitProperty_CurrentDevice,
//									   kAudioUnitScope_Global,
//									   0,
//									   &state->deviceOut,
//									   sizeof(state->deviceOut));
//
//	if (result != noErr) {
//		throw std::runtime_error("CoreAudio: failed to set output device with error " + std::to_string(result));
//	}
//}

//void CoreAudioSystem::enableAudioIO() {
//	UInt32 enableOutput = (state->outChans > 0) ? 1 : 0;
//	AudioUnitSetProperty(state->audioUnit,
//						 kAudioOutputUnitProperty_EnableIO,
//						 kAudioUnitScope_Output,
//						 0,
//						 &enableOutput,
//						 sizeof(enableOutput));
//
//	UInt32 enableInput = (state->inChans > 0) ? 1 : 0;
//	AudioUnitSetProperty(state->audioUnit,
//						 kAudioOutputUnitProperty_EnableIO,
//						 kAudioUnitScope_Input,
//						 1,
//						 &enableInput,
//						 sizeof(enableInput));
//}

//void CoreAudioSystem::setupStreamFormat() {
//	state->fmt				= {};
//	state->fmt.mSampleRate	= sampleRate;
//	state->fmt.mFormatID	= kAudioFormatLinearPCM;
//	state->fmt.mFormatFlags = static_cast<UInt32>(kAudioFormatFlagIsFloat)
//							  | static_cast<UInt32>(kAudioFormatFlagsNativeEndian)
//							  | static_cast<UInt32>(kAudioFormatFlagIsPacked);
//	state->fmt.mBitsPerChannel	= sizeof(float) * 8;
//	state->fmt.mFramesPerPacket = 1;
//
//	if (state->outChans > 0) {
//		AudioStreamBasicDescription outFmt = state->fmt;
//		outFmt.mChannelsPerFrame		   = static_cast<UInt32>(state->outChans.load());
//		outFmt.mBytesPerFrame			   = sizeof(float) * outFmt.mChannelsPerFrame;
//		outFmt.mBytesPerPacket			   = outFmt.mBytesPerFrame * outFmt.mFramesPerPacket;
//
//		auto result = AudioUnitSetProperty(
//			state->audioUnit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input, 0, &outFmt, sizeof(outFmt));
//		if (result != noErr) {
//			throw std::runtime_error("CoreAudio: set output stream format failed with error "
//									 + std::to_string(result));
//		}
//	}
//
//	if (state->inChans > 0) {
//		AudioStreamBasicDescription inFmt = state->fmt;
//		inFmt.mChannelsPerFrame			  = static_cast<UInt32>(state->inChans.load());
//		inFmt.mBytesPerFrame			  = sizeof(float) * inFmt.mChannelsPerFrame;
//		inFmt.mBytesPerPacket			  = inFmt.mBytesPerFrame * inFmt.mFramesPerPacket;
//
//		auto result = AudioUnitSetProperty(
//			state->audioUnit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Output, 1, &inFmt, sizeof(inFmt));
//		if (result != noErr) {
//			throw std::runtime_error("CoreAudio: set input stream format failed with error "
//									 + std::to_string(result));
//		}
//	}
//
//	if (state->deviceOut != kAudioObjectUnknown) {
//		AudioObjectPropertyAddress addr {
//			kAudioDevicePropertyBufferFrameSize, kAudioDevicePropertyScopeOutput, kAudioObjectPropertyElementMain};
//		auto frames = static_cast<UInt32>(bufferSize);
//		UInt32 size = sizeof(frames);
//		AudioObjectSetPropertyData(state->deviceOut, &addr, 0, nullptr, size, &frames);
//	}
//
//	auto maxFrames = static_cast<UInt32>(bufferSize);
//	AudioUnitSetProperty(state->audioUnit,
//						 kAudioUnitProperty_MaximumFramesPerSlice,
//						 kAudioUnitScope_Global,
//						 0,
//						 &maxFrames,
//						 sizeof(maxFrames));
//}

//void CoreAudioSystem::setupAudioBuffers() {
//	auto maxFrames = static_cast<UInt32>(bufferSize);
//	auto size	   = static_cast<UInt32>(sizeof(maxFrames));
//	AudioUnitSetProperty(
//		state->audioUnit, kAudioUnitProperty_MaximumFramesPerSlice, kAudioUnitScope_Global, 0, &maxFrames, size);
//
//	if (state->inChans > 0) {
//		const size_t samples = static_cast<size_t>(maxFrames) * static_cast<size_t>(state->inChans);
//		state->inputInterleaved.assign(samples, 0.0f);
//		state->inputBufferCapacityFrames = maxFrames;
//	}
//}

//void CoreAudioSystem::setupCallback() {
//	AURenderCallbackStruct callback {};
//	callback.inputProc		 = renderProc;
//	callback.inputProcRefCon = this;
//	auto result				 = AudioUnitSetProperty(state->audioUnit,
//										kAudioUnitProperty_SetRenderCallback,
//										kAudioUnitScope_Input,
//										0,
//										&callback,
//										sizeof(callback));
//	if (result != noErr) {
//		throw std::runtime_error("CoreAudio: set render callback failed with error " + std::to_string(result));
//	}
//
//	result = AudioUnitInitialize(state->audioUnit);
//	if (result != noErr) {
//		throw std::runtime_error("CoreAudio: AudioUnitInitialize failed with error " + std::to_string(result));
//	}
//}

void CoreAudioSystem::setup(int numInChannels, int numOutChannels) {
	if (state != nullptr) {
		Log::d() << "CoreAudioSystem is running, stopping before setup";
		stop();
	}

	try {
		setupState(numInChannels, numOutChannels);
		createInputAudioUnit();
		createOutputAudioUnit();
		//		connectOutput();
		//		enableAudioIO();
		//		setupStreamFormat();
		//		setupAudioBuffers();
		//		setupCallback();
	} catch (...) {
		Log::e() << "CoreAudioSystem setup failed";
		throw;
	}
}

void CoreAudioSystem::start() {
	if (state == nullptr) {
		Log::e() << "CoreAudioSystem state is null, cannot start. You must call setup(int, int) first.";
		return;
	}

	if (state->running.load()) {
		Log::e() << "CoreAudioSystem is already running, not starting again";
		return;
	}

	OSStatus startedIn	= noErr;
	OSStatus startedOut = noErr;

	if (state->audioUnitIn != nullptr) {
		startedIn = AudioOutputUnitStart(state->audioUnitIn);
		if (startedIn != noErr) {
			Log::e() << "CoreAudioSystem failed to start audio unit (in) with error " << startedIn;
		}
	}

	if (state->audioUnitOut != nullptr) {
		startedOut = AudioOutputUnitStart(state->audioUnitOut);
		if (startedOut != noErr) {
			Log::e() << "CoreAudioSystem failed to start audio unit (out) with error " << startedOut;
		}
	}

	state->running.store(startedIn == noErr && startedOut == noErr);
}

void CoreAudioSystem::stop() {
	if (state == nullptr) {
		Log::e() << "CoreAudioSystem state is null, cannot stop";
		return;
	}

	if (!state->running.load()) {
		Log::e() << "CoreAudioSystem is not running, so cannot be stopped";
		return;
	}

	if (state->audioUnitIn != nullptr) {
		auto result = AudioOutputUnitStop(state->audioUnitIn);
		if (result != noErr) {
			Log::e() << "CoreAudioSystem failed to stop audio unit (in) with error " << result;
		}
	}

	if (state->audioUnitOut != nullptr) {
		auto result = AudioOutputUnitStop(state->audioUnitOut);
		if (result != noErr) {
			Log::e() << "CoreAudioSystem failed to start audio unit (out) with error " << result;
		}
	}

	state = nullptr;
}

void CoreAudioSystem::restart() {
	if (!isRunning()) {
		return;
	}

	auto in	 = state->inChans.load();
	auto out = state->outChans.load();

	stop();
	setup(in, out);
	start();
}

bool CoreAudioSystem::isRunning() {
	return state ? state->running.load() : false;
}

bool CoreAudioSystem::isInsideAudioCallback() {
	return state ? state->insideCallback.load() : false;
}

void CoreAudioSystem::setVerbose(bool _verbose) {
	verbose = _verbose;
}

std::vector<AudioPort> CoreAudioSystem::getInputs() {
	return inputPorts;
}

std::vector<AudioPort> CoreAudioSystem::getOutputs() {
	return outputPorts;
}

bool CoreAudioSystem::setInput(const AudioPort &audioInput) {
	inputPort = audioInput;
	updateRunningParameters();
	restart();
	return true;
}

bool CoreAudioSystem::setOutput(const AudioPort &audioOutput) {
	outputPort = audioOutput;
	updateRunningParameters();
	restart();
	return true;
}

void CoreAudioSystem::setSampleRate(float _sampleRate) {
	sampleRate = _sampleRate;
	restart();
}

void CoreAudioSystem::setBufferSize(int _size) {
	bufferSize = _size;
	restart();
}

void CoreAudioSystem::updateRunningParameters() {
	sampleRate = getPreferredSampleRate();
	bufferSize = getPreferredNumberOfFrames();
}

void CoreAudioSystem::updateDefaultIOPorts() {
	bool changedPorts = false;
	if (!inputPort.has_value()) {
		for (const auto &port: inputPorts) {
			if (port.isDefaultInput) {
				inputPort	 = port;
				changedPorts = true;
				break;
			}
		}
	}

	if (!outputPort.has_value()) {
		for (const auto &port: outputPorts) {
			if (port.isDefaultOutput) {
				outputPort	 = port;
				changedPorts = true;
				break;
			}
		}
	}

	if (changedPorts) {
		updateRunningParameters();
	}
}

void CoreAudioSystem::rescanPorts() {
	getDeviceList(inputPorts, outputPorts);
	updateDefaultIOPorts();
	if (verbose) {
		printDeviceList();
	}
}

void CoreAudioSystem::printDeviceList() const {
	Log::d() << "CoreAudioSystem: Input Devices:";
	for (const auto &port: inputPorts) {
		Log::d() << port.toString();
	}

	Log::d() << "CoreAudioSystem: Output Devices:";
	for (const auto &port: outputPorts) {
		Log::d() << port.toString();
	}
}

AudioPort CoreAudioSystem::getInput() {
	if (inputPort.has_value()) {
		return *inputPort;
	}
	return {};
}

AudioPort CoreAudioSystem::getOutput() {
	if (outputPort.has_value()) {
		return *outputPort;
	}
	return {};
}

double CoreAudioSystem::getOutputLatency() {
	if (state == nullptr) {
		return 0.0;
	}

	auto frames = getDeviceLatency(state->deviceOut, DeviceType::Output);
	if (!frames.has_value()) {
		return 0.0;
	}

	return *frames / sampleRate;
}

double CoreAudioSystem::getNanoSecondsAtBufferBegin() {
	return state == nullptr ? 0.0 : static_cast<double>(hostTimeToNanos(state->lastBufferBeginHostTime.load()));
}

double CoreAudioSystem::getHostTime() {
	return static_cast<double>(hostTimeToNanos(AudioGetCurrentHostTime()));
}

CoreAudioState &CoreAudioSystem::getState() {
	if (state == nullptr) {
		throw std::runtime_error("CoreAudioSystem state is null, cannot get state");
	}
	return *state;
}