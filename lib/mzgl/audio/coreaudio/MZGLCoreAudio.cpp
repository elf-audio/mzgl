#include "MZGLCoreAudio.h"

#include <atomic>
#include <vector>
#include <string>
#include <cstring>
#include <algorithm>

#include "MZGLCoreAudioDeviceHelpers.h"
#include "MZGLCoreAudioHelpers.h"
#include "MZGLCoreAudioAggregrateHelpers.h"
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

	mzAssert(inputPort.has_value() && outputPort.has_value());

	state			= std::make_unique<CoreAudioState>();
	state->inChans	= std::max(0, numInChannels);
	state->outChans = std::max(0, numOutChannels);

	auto aggName = "mzgl_aggregate_" + outputPort->name + "_" + inputPort->name;

	auto agg = createAggregateDevice(inputPort->portId, outputPort->portId, aggName);
	if (agg.has_value()) {
		state->deviceOut = *agg;
		state->deviceIn	 = *agg;
	} else {
		Log::e() << "CoreAudio: could not create aggregate device for input: " << inputPort->name
				 << " and output: " << outputPort->name;
		state->deviceOut = outputPort.has_value() ? outputPort->portId : kAudioObjectUnknown;
		state->deviceIn	 = inputPort.has_value() ? inputPort->portId : kAudioObjectUnknown;
	}
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
	state->fmt.mSampleRate	= sampleRate;
	state->fmt.mFormatID	= kAudioFormatLinearPCM;
	state->fmt.mFormatFlags = static_cast<UInt32>(kAudioFormatFlagIsFloat)
							  | static_cast<UInt32>(kAudioFormatFlagsNativeEndian)
							  | static_cast<UInt32>(kAudioFormatFlagIsPacked);
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
		AudioObjectPropertyAddress addr {
			kAudioDevicePropertyBufferFrameSize, kAudioDevicePropertyScopeOutput, kAudioObjectPropertyElementMain};
		auto frames = static_cast<UInt32>(bufferSize);
		UInt32 size = sizeof(frames);
		AudioObjectSetPropertyData(state->deviceOut, &addr, 0, nullptr, size, &frames);
	}

	auto maxFrames = static_cast<UInt32>(bufferSize);
	AudioUnitSetProperty(state->audioUnit,
						 kAudioUnitProperty_MaximumFramesPerSlice,
						 kAudioUnitScope_Global,
						 0,
						 &maxFrames,
						 sizeof(maxFrames));
}

void CoreAudioSystem::setupAudioBuffers() {
	auto maxFrames = static_cast<UInt32>(bufferSize);
	auto size	   = static_cast<UInt32>(sizeof(maxFrames));
	AudioUnitSetProperty(
		state->audioUnit, kAudioUnitProperty_MaximumFramesPerSlice, kAudioUnitScope_Global, 0, &maxFrames, size);

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
	if (state != nullptr) {
		Log::d() << "CoreAudioSystem is running, stopping before setup";
		stop();
	}

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
		Log::e() << "CoreAudioSystem state is null, cannot start. You must call setup(int, int) first.";
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