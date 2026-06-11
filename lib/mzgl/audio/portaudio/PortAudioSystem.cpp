//
//  AudioSystem.cpp
//  MZGL
//
//  Created by Marek Bereza on 23/01/2018.
//  Copyright © 2018 Marek Bereza. All rights reserved.
//

#include "PortAudioSystem.h"
#include "log.h"
// #ifdef __APPLE__
// #import <AVFoundation/AVFoundation.h>
// #endif
#include "portaudio.h"
#ifdef __linux__
#	include "pa_linux_alsa.h"
#endif
#include <algorithm>
#include <cstdio>

using namespace std;

#ifdef _WIN32
#	include <excpt.h>
// Pa_Initialize()'s ASIO host-api setup LoadLibrary()s every registered ASIO
// driver in-process to query it, so a broken third-party driver (e.g. the
// FlexASIO crash-loop seen in June 2026 crash reports) can fault before the UI
// even appears. Isolate the call under SEH so the caller can log a diagnostic
// instead of Koala dying silently at startup. Kept free of C++ objects:
// __try can't share a frame with unwinding (C2712).
static PaError paInitializeGuarded(unsigned long *sehCode) noexcept {
	__try {
		return Pa_Initialize();
	} __except (EXCEPTION_EXECUTE_HANDLER) {
		*sehCode = GetExceptionCode();
		return paInternalError;
	}
}
#endif

bool PortAudioSystem::checkPaError(PaError err, string msg) {
	if (err != paNoError) {
		Log::e() << "PortAudio error: " << msg << " - error is " << err << "(" << Pa_GetErrorText(err) << ")";
		return false;
	} else if (verbose) {
		Log::d() << "PortAudioSystem Success " << msg;
	}

	return true;
}

PortAudioSystem::PortAudioSystem() {
	if (verbose) {
		Log::d() << "PortAudioSystem()";
		printf("PortAudioSystem()\n");
	}
#ifdef _WIN32
	unsigned long sehCode = 0;
	auto err			  = paInitializeGuarded(&sehCode);
	if (sehCode != 0) {
		char codeStr[32];
		snprintf(codeStr, sizeof(codeStr), "0x%08lX", sehCode);
		Log::e() << "Caught SEH exception " << codeStr
				 << " inside Pa_Initialize() - a third-party audio driver (probably an ASIO driver) "
					"crashed while being enumerated";
	}
#else
	auto err = Pa_Initialize();
#endif
	if (!checkPaError(err, "Intializing port audio")) {
		throw std::runtime_error("dang! portaudio not working");
	}
#ifdef _WIN32
	// Default to WASAPI on Windows for lowest latency
	auto wasapiIndex = Pa_HostApiTypeIdToHostApiIndex(paWASAPI);
	if (wasapiIndex >= 0) {
		hostApiTypeId = paWASAPI;
	}
#endif
	rescanPorts();
}

PortAudioSystem::~PortAudioSystem() {
	if (verbose) {
		Log::e() << "Tearing down PortAudioSystem";
	}
	closeStream();
	auto err = Pa_Terminate();
	checkPaError(err, "terminating");
}

void PortAudioSystem::closeStream() {
	if (stream != nullptr) {
		if (!Pa_IsStreamStopped(stream)) {
			auto err = Pa_StopStream(stream);
			checkPaError(err, "stopping stream");
		}
		auto err = Pa_CloseStream(stream);
		checkPaError(err, "closing stream");
		stream = nullptr;
	}
}

double PortAudioSystem::getNanoSecondsAtBufferBegin() {
	return outputTime;
}

void PortAudioSystem::startAudioCallback() {
	inProcess.store(true);
}

void PortAudioSystem::finishedAudioCallback() {
	inProcess.store(false);
}

static int PortAudioSystem_callback(const void *inputBuffer,
									void *outputBuffer,
									unsigned long framesPerBuffer,
									const PaStreamCallbackTimeInfo *timeInfo,
									PaStreamCallbackFlags statusFlags,
									void *userData) {
	PortAudioSystem *as = (PortAudioSystem *) userData;
	as->startAudioCallback();

	as->inputTime =
		timeInfo
			->inputBufferAdcTime; /**< The time when the first sample of the input buffer was captured at the ADC input */
	as->outputTime = timeInfo->outputBufferDacTime * 1e9;

	if (inputBuffer != nullptr) {
		as->inputCallback((float *) inputBuffer, (int) framesPerBuffer, as->numInChannels);
	}

	if (outputBuffer != nullptr) {
		as->outputCallback((float *) outputBuffer, (int) framesPerBuffer, as->numOutChannels);
	}

	as->finishedAudioCallback();
	return paContinue;
}

//#ifdef __APPLE__
//double getMacDefaultDeviceSampleRate() {
//
//
//
//	AudioObjectID      deviceID;
//
//	// load the current default device
//	UInt32 deviceSize = sizeof(deviceID);
//	AudioObjectPropertyAddress address = { kAudioHardwarePropertyDefaultInputDevice, kAudioObjectPropertyScopeGlobal,  kAudioObjectPropertyElementMaster};
//
//	auto err = AudioObjectGetPropertyData(kAudioObjectSystemObject, &address, 0, NULL, &deviceSize, &deviceID);
//
//
//	if(err != kAudioHardwareNoError) {
//		NSLog(@"Error getting default device");
//		return 0;
//	}
//
//
//	AudioObjectPropertyAddress addr;
//
//	addr.mSelector = kAudioDevicePropertyNominalSampleRate;
//	addr.mScope = kAudioObjectPropertyScopeGlobal;
//	addr.mElement = kAudioObjectPropertyElementMaster;
//
//	UInt32 dataSize = 0;
//	err = AudioObjectGetPropertyDataSize(deviceID, &addr, 0, NULL, &dataSize);
//
//
//	if(err != kAudioHardwareNoError) {
//		NSLog(@"Error getting prop size");
//		return 0;
//	}
//
//	double val;
//
//
//	err = AudioObjectGetPropertyData( deviceID,
//								&addr,
//								0,
//								NULL,
//								&dataSize,
//								&val);
//
//	if(err != kAudioHardwareNoError) {
//		NSLog(@"Error getting prop");
//		return 0;
//	}
////	this->sampleRate = val;
////					this->sampleRate = outSampleRate;
////			NSLog(@"Want %f Hz", this->sampleRate);
//
//	return val;
//}
//#endif

AudioPort PortAudioSystem::getPort(int dev) {
	if (verbose) {
		Log::d() << "Gettting port for " << dev;
	}
	for (auto &p: ports) {
		if (p.portId == dev) {
			return p;
		}
	}
	return AudioPort();
}

bool PortAudioSystem::setInput(const AudioPort &audioInput) {
	return setIOPort(audioInput, false);
}

bool PortAudioSystem::setOutput(const AudioPort &audioOutput) {
	return setIOPort(audioOutput, true);
}

bool PortAudioSystem::setIOPort(const AudioPort &audioPort, bool isOutput) {
	if (verbose) {
		Log::d() << "Setting " << (isOutput ? "output" : "input") << " to " << audioPort.name;
	}

	bool success			= true;
	bool shouldStopAndStart = isRunning();
	if (shouldStopAndStart) {
		stop();
	}

	if (isOutput) {
		outPort = audioPort;
	} else {
		inPort		  = audioPort;
		isInPortDummy = (inPort.numInChannels == 0);
	}

	if (setupFinished) {
		sampleRate = 0; // reset sample rate to port-default (uses output's default first)
		configureStream();

		// for input changes, retry once with the input's own native sample rate
		// — the most common failure is shared-mode WASAPI mix-format mismatch
		if (!isOutput && streamConfigStatus_ != StreamConfigurationStatus::OK
			&& inPort.defaultSampleRate > 0) {
			sampleRate = inPort.defaultSampleRate;
			configureStream();
		}

		// still failing? try switching the output to the same device as the input
		// — duplex interfaces (USB audio etc.) always agree with themselves
		if (!isOutput && streamConfigStatus_ != StreamConfigurationStatus::OK) {
			AudioPort matchedOut;
			if (inPort.numOutChannels > 0) {
				matchedOut = inPort;
			} else {
				for (const auto &p: ports) {
					if (p.numOutChannels > 0 && p.name == inPort.name) {
						matchedOut = p;
						break;
					}
				}
			}
			if (matchedOut.isValid()) {
				outPort	   = matchedOut;
				sampleRate = 0;
				configureStream();
			}
		}

		// fallback to no-input
		if (streamConfigStatus_ != StreamConfigurationStatus::OK) {
			sampleRate = 0; // reset sample rate to port-default
			setNoInputPort();
			configureStream();
			success =
				false; // even if configureStream() succeeded, we return false to inform that input is set to no-input
		}

		if (streamConfigStatus_ != StreamConfigurationStatus::OK) {
			// TODO: shall never happen, really don't know what to do in such case, TBD
			Log::e() << "PANIC: cannot configure stream with no-input!";
		}
	}

	if (shouldStopAndStart) {
		start();
	}

	return success;
}

void PortAudioSystem::setup(int numIns, int numOuts) {
	if (verbose) {
		Log::d() << "PortAudioSystem::setup(" << numIns << ", " << numOuts << ")";
	}
	this->numInChannels			= numIns;
	this->numOutChannels		= numOuts;
	this->desiredNumInChannels	= numIns;
	this->desiredNumOutChannels = numOuts;

	configureStream();

	// in case of error fallback to no-input
	if (streamConfigStatus_ != StreamConfigurationStatus::OK) {
		setNoInputPort();
		configureStream();
	}

	// last resort: the selected host API's default devices may be unusable
	// (e.g. WASAPI reporting paNoDevice for defaultOutputDevice on a fresh
	// install). Drop the host API preference and let PortAudio pick — this
	// gives us a working stream out of the box; the user can switch host API
	// later from the audio settings page.
	if (streamConfigStatus_ != StreamConfigurationStatus::OK && hostApiTypeId >= 0) {
		Log::e() << "PortAudioSystem: configured host API can't open a stream; "
					"falling back to PortAudio's default host API.";
		hostApiTypeId = -1;
		inPort		  = AudioPort();
		outPort		  = AudioPort();
		isInPortDummy = false;
		sampleRate	  = 0;
		configureStream();
		if (streamConfigStatus_ != StreamConfigurationStatus::OK) {
			setNoInputPort();
			configureStream();
		}
	}

	// however configureStream might fail at that point, but number of desired channels is already set
	// so mark that setup finished, upper layer shall check for errors and decide what to do next
	setupFinished = true;
}

void PortAudioSystem::configureStream() {
	if (verbose) {
		Log::d() << "PortAudioSystem::configureStream()";
	}

	closeStream();

	numInChannels  = isInPortDummy ? 0 : desiredNumInChannels;
	numOutChannels = desiredNumOutChannels;

	// just make sure our ports are up to date
	rescanPorts();
	PaStreamParameters inputParameters, outputParameters;

	// determine default devices for the selected host API
	int defaultInputDeviceId  = Pa_GetDefaultInputDevice();
	int defaultOutputDeviceId = Pa_GetDefaultOutputDevice();
	if (hostApiTypeId >= 0) {
		auto apiIndex = Pa_HostApiTypeIdToHostApiIndex(static_cast<PaHostApiTypeId>(hostApiTypeId));
		if (apiIndex >= 0) {
			auto *apiInfo		  = Pa_GetHostApiInfo(apiIndex);
			defaultInputDeviceId  = apiInfo->defaultInputDevice;
			defaultOutputDeviceId = apiInfo->defaultOutputDevice;
		}
	}

	if (this->numInChannels > 0) {
		if (!inPort.isValid()) {
			inPort		  = getPort(defaultInputDeviceId);
			isInPortDummy = false;
		}
		inputParameters.device = inPort.portId;
		if (inputParameters.device == paNoDevice) {
			Log::e() << "Error: No default input device.";
			numInChannels = 0;
			setNoInputPort();
			//return;
		} else {
			numInChannels					 = min(inPort.numInChannels, numInChannels);
			inputParameters.channelCount	 = numInChannels;
			inputParameters.sampleFormat	 = paFloat32;
			inputParameters.suggestedLatency = Pa_GetDeviceInfo(inputParameters.device)->defaultLowInputLatency;
			inputParameters.hostApiSpecificStreamInfo = nullptr;
		}
	} else if (this->numInChannels == 0) {
		setNoInputPort();
	}

	if (this->numOutChannels > 0) {
		if (!outPort.isValid()) {
			outPort = getPort(defaultOutputDeviceId);
		}

		outputParameters.device = outPort.portId;
		if (outputParameters.device == paNoDevice) {
			Log::e() << "Error: No default output device.";
			streamConfigStatus_ = StreamConfigurationStatus::FAILED;
			return;
		}
		numOutChannels				  = min((int) outPort.numOutChannels, numOutChannels);
		outputParameters.channelCount = numOutChannels;

		outputParameters.sampleFormat	  = paFloat32;
		outputParameters.suggestedLatency = Pa_GetDeviceInfo(outputParameters.device)->defaultLowOutputLatency;
		outputParameters.hostApiSpecificStreamInfo = nullptr;
	}

	if (sampleRate == 0) {
		if (this->numOutChannels > 0) {
			sampleRate = outPort.defaultSampleRate;
			//            Log::d() << "Using default output sample rate of " << outPort.defaultSampleRate;
		} else if (this->numInChannels > 0) {
			sampleRate = inPort.defaultSampleRate;
		}
	} else {
		Log::d() << "Using user specified sample rate of " << sampleRate;
	}
	if (verbose) {
		Log::d() << "---------------------------------------------------------";
		for (auto &p: ports) {
			Log::d() << "PORT: " << p.toString();
		}
	}

	auto *inParams	= &inputParameters;
	auto *outParams = &outputParameters;

	if (numInChannels < 1) inParams = nullptr;
	if (numOutChannels < 1) outParams = nullptr;

	if (verbose) {
		Log::d() << "Calling Pa_OpenStream";
	}

	auto err =
		Pa_OpenStream(&stream,
					  inParams,
					  outParams,
					  sampleRate,
					  bufferSize,
					  0,
					  /* paClipOff, */ /* we won't output out of range samples so don't bother clipping them */
					  PortAudioSystem_callback,
					  this);
	if (!checkPaError(err, "open stream")) {
		streamConfigStatus_ = StreamConfigurationStatus::FAILED;
		return;
	} else if (verbose) {
		Log::d() << "Success opening stream";
	}

#ifdef __linux__
	PaAlsa_EnableRealtimeScheduling(stream, true);
#endif
	streamConfigStatus_ = StreamConfigurationStatus::OK;

	// when asking for a specific sample rate, we don't
	// always get it, so we need to update the sampleRate
	// value to reflect that and make a warning for now
	const auto *info = Pa_GetStreamInfo(stream);
	if (info != nullptr) {
		if (info->sampleRate != sampleRate) {
			Log::w() << "Didn't get desired samplerate of " << to_string(sampleRate / 1000.f, 0) << "kHz, got "
					 << to_string(info->sampleRate / 1000.f, 0) << "kHz instead";
			sampleRate = info->sampleRate;
		}
	}
}
void PortAudioSystem::start() {
	if (stream == nullptr) return;
	auto err = Pa_StartStream(stream);
	checkPaError(err, "start stream");
	Log::d() << to_string(getOutputLatency() * 1000.0, 0)
			 << "ms output latency, buffersize: " << std::to_string(bufferSize) << " samples";
}

void PortAudioSystem::stop() {
	if (stream == nullptr) return;
	auto err = Pa_StopStream(stream);
	checkPaError(err, "stop stream");
}

bool PortAudioSystem::isRunning() {
	if (stream == nullptr) return false;
	if (verbose) {
		Log::d() << "PortAudioSystem::isRunning()" << Pa_IsStreamActive(stream);
	}
	return Pa_IsStreamActive(stream) == 1;
}

bool PortAudioSystem::isInsideAudioCallback() {
	return inProcess.load();
}

vector<AudioPort> PortAudioSystem::getOutputs() {
	//	updatePorts();
	vector<AudioPort> ret;
	for (const auto &p: ports) {
		if (p.numOutChannels > 0) ret.push_back(p);
	}
	return ret;
}

AudioPort PortAudioSystem::getDummyInputPort() {
	AudioPort noInput;
	noInput.name = "No Input";
	return noInput;
}

vector<AudioPort> PortAudioSystem::getInputs() {
	//	updatePorts();
	vector<AudioPort> ret;

	// set "No Input" first on the list
	ret.push_back(getDummyInputPort());

	for (const auto &p: ports) {
		if (p.numInChannels > 0) ret.push_back(p);
	}
	return ret;
}

void PortAudioSystem::rescanPorts() {
	if (verbose) {
		Log::d() << "PortAudioSystem::rescanPorts()";
	}
	ports.clear();

	int defaultInputDeviceId  = Pa_GetDefaultInputDevice();
	int defaultOutputDeviceId = Pa_GetDefaultOutputDevice();

	// if we have a selected host API, use its defaults
	int selectedApiIndex = -1;
	if (hostApiTypeId >= 0) {
		selectedApiIndex = Pa_HostApiTypeIdToHostApiIndex(static_cast<PaHostApiTypeId>(hostApiTypeId));
		if (selectedApiIndex >= 0) {
			auto *apiInfo		  = Pa_GetHostApiInfo(selectedApiIndex);
			defaultInputDeviceId  = apiInfo->defaultInputDevice;
			defaultOutputDeviceId = apiInfo->defaultOutputDevice;
		}
	}

	int numDevices = Pa_GetDeviceCount();
	if (numDevices < 0) {
		Log::e() << "Couldn't get number of devices from PortAudio! - count was " << numDevices;
		return;
	}

	for (int i = 0; i < numDevices; i++) {
		auto dev = Pa_GetDeviceInfo(i);

		// filter by host API if one is selected
		if (selectedApiIndex >= 0 && dev->hostApi != selectedApiIndex) {
			continue;
		}

		AudioPort port;
		port.portId			= i;
		port.name			= dev->name;
		port.numInChannels	= dev->maxInputChannels;
		port.numOutChannels = dev->maxOutputChannels;

		if (i == defaultInputDeviceId) {
			port.isDefaultInput = true;
		}
		if (i == defaultOutputDeviceId) {
			port.isDefaultOutput = true;
		}

		port.defaultSampleRate = dev->defaultSampleRate;

		ports.emplace_back(port);
	}
}
double PortAudioSystem::getLatency() {
	const auto *info = Pa_GetStreamInfo(stream);
	if (info != nullptr) {
		return info->inputLatency + info->outputLatency;
	}
	return 0.0;
}

double PortAudioSystem::getOutputLatency() {
	const auto *info = Pa_GetStreamInfo(stream);
	if (info != nullptr) {
		return info->outputLatency;
	}
	return 0.0;
}

double PortAudioSystem::getHostTime() {
	return hostTime;
}

void PortAudioSystem::setSampleRate(float sr) {
	bool wasRunning = isRunning();
	closeStream();
	this->sampleRate = sr;
	configureStream();
	if (wasRunning && streamConfigStatus_ == StreamConfigurationStatus::OK) {
		start();
	}
	notifySampleRateChanged();
}

void PortAudioSystem::setBufferSize(int size) {
	bool wasRunning = isRunning();
	closeStream();
	this->bufferSize = size;
	configureStream();
	if (wasRunning && streamConfigStatus_ == StreamConfigurationStatus::OK) {
		start();
	}
}

static HostApi paTypeToHostApi(PaHostApiTypeId t) {
	switch (t) {
		case paMME: return HostApi::MME;
		case paDirectSound: return HostApi::DirectSound;
		case paASIO: return HostApi::ASIO;
		case paSoundManager: return HostApi::SoundManager;
		case paCoreAudio: return HostApi::CoreAudio;
		case paOSS: return HostApi::OSS;
		case paALSA: return HostApi::ALSA;
		case paAL: return HostApi::AL;
		case paBeOS: return HostApi::BeOS;
		case paWDMKS: return HostApi::WDMKS;
		case paJACK: return HostApi::JACK;
		case paWASAPI: return HostApi::WASAPI;
		case paAudioScienceHPI: return HostApi::AudioScienceHPI;
		default: return HostApi::Unknown;
	}
}

static int hostApiToPaType(HostApi k) {
	switch (k) {
		case HostApi::MME: return paMME;
		case HostApi::DirectSound: return paDirectSound;
		case HostApi::ASIO: return paASIO;
		case HostApi::SoundManager: return paSoundManager;
		case HostApi::CoreAudio: return paCoreAudio;
		case HostApi::OSS: return paOSS;
		case HostApi::ALSA: return paALSA;
		case HostApi::AL: return paAL;
		case HostApi::BeOS: return paBeOS;
		case HostApi::WDMKS: return paWDMKS;
		case HostApi::JACK: return paJACK;
		case HostApi::WASAPI: return paWASAPI;
		case HostApi::AudioScienceHPI: return paAudioScienceHPI;
		case HostApi::Unknown:
		case HostApi::AudioIO: return -1;
	}
	return -1;
}

#ifdef _WIN32
static std::string stripWindowsPrefix(const char *name) {
	std::string s			 = name ? name : "";
	const std::string prefix = "Windows ";
	if (s.rfind(prefix, 0) == 0) s.erase(0, prefix.size());
	return s;
}

static bool isUserFacingHostApi(PaHostApiTypeId type) {
	return type == paWASAPI || type == paASIO || type == paDirectSound;
}
#endif

static AudioHostApi makeAudioHostApi(const PaHostApiInfo *info) {
	AudioHostApi api;
	api.typeId = info->type;
	api.kind   = paTypeToHostApi(static_cast<PaHostApiTypeId>(info->type));
#ifdef _WIN32
	api.name = stripWindowsPrefix(info->name);
#else
	api.name = info->name ? info->name : "";
#endif
	return api;
}

vector<AudioHostApi> PortAudioSystem::getAvailableHostApis() {
	vector<AudioHostApi> apis;
	int count = Pa_GetHostApiCount();
	for (int i = 0; i < count; i++) {
		auto *info = Pa_GetHostApiInfo(i);
		if (info == nullptr || info->deviceCount <= 0) continue;
#ifdef _WIN32
		if (!isUserFacingHostApi(static_cast<PaHostApiTypeId>(info->type))) continue;
#endif
		apis.push_back(makeAudioHostApi(info));
	}
	return apis;
}

AudioHostApi PortAudioSystem::getHostApi() {
	if (hostApiTypeId >= 0) {
		auto apiIndex = Pa_HostApiTypeIdToHostApiIndex(static_cast<PaHostApiTypeId>(hostApiTypeId));
		if (apiIndex >= 0) {
			return makeAudioHostApi(Pa_GetHostApiInfo(apiIndex));
		}
	}
	return {};
}

void PortAudioSystem::setHostApi(HostApi kind) {
	int typeId = hostApiToPaType(kind);
	if (typeId < 0) return;
	if (typeId == hostApiTypeId) return;

	bool wasRunning = isRunning();
	closeStream();

	hostApiTypeId = typeId;

	// invalidate current ports so configureStream picks new defaults
	inPort	= AudioPort();
	outPort = AudioPort();
	isInPortDummy = false;
	sampleRate	  = 0;

	if (setupFinished) {
		configureStream();
		if (streamConfigStatus_ != StreamConfigurationStatus::OK) {
			setNoInputPort();
			configureStream();
		}
		if (wasRunning && streamConfigStatus_ == StreamConfigurationStatus::OK) {
			start();
		}
	}

	notifySampleRateChanged();
}

vector<double> PortAudioSystem::getAvailableSampleRates() {
	const vector<double> candidates = {22050.0, 44100.0, 48000.0, 88200.0, 96000.0, 176400.0, 192000.0};
	vector<double> supported;

	// check against current output device
	int devId = outPort.isValid() ? outPort.portId : Pa_GetDefaultOutputDevice();
	if (devId == paNoDevice) return {44100.0, 48000.0};

	auto *devInfo = Pa_GetDeviceInfo(devId);
	if (devInfo == nullptr) return {44100.0, 48000.0};

	for (auto sr: candidates) {
		PaStreamParameters params;
		params.device					  = devId;
		params.channelCount				  = min(devInfo->maxOutputChannels, 2);
		params.sampleFormat				  = paFloat32;
		params.suggestedLatency			  = 0;
		params.hostApiSpecificStreamInfo = nullptr;

		if (params.channelCount > 0) {
			auto err = Pa_IsFormatSupported(nullptr, &params, sr);
			if (err == paFormatIsSupported) {
				supported.push_back(sr);
			}
		}
	}

	if (supported.empty()) {
		supported = {44100.0, 48000.0};
	}
	return supported;
}

vector<int> PortAudioSystem::getAvailableBufferSizes() {
	return {64, 128, 256, 512, 1024, 2048, 4096};
}
