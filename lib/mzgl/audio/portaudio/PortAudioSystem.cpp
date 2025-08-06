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

using namespace std;

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
	}

	auto err = Pa_Initialize();
	if (!checkPaError(err, "Intializing port audio")) {
		throw std::runtime_error("dang! portaudio not working");
	}

	if (verbose) {
		int numApis = Pa_GetHostApiCount();
		if (numApis == paNotInitialized) {
			Log::e() << "Portaudio not initizliaed";
		}
		Log::d() << "Listing Host API's (" << numApis << ")";
		for (int i = 0; i < numApis; ++i) {
			const PaHostApiInfo *apiInfo = Pa_GetHostApiInfo(i);
			Log::d() << "> API " << apiInfo->name;
		}
	}

	rescanPorts();
}

PortAudioSystem::~PortAudioSystem() {
	if (verbose) {
		Log::e() << "Tearing down PortAudioSystem";
	}
	if (isRunning()) {
		stop();
	}
	if (stream != nullptr && !Pa_IsStreamStopped(stream)) {
		auto err = Pa_StopStream(stream);
		checkPaError(err, "stopping");
	}
	auto err = Pa_Terminate();
	checkPaError(err, "terminating");
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
		sampleRate = 0; // reset sample rate to port-default
		configureStream();

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

	// however configureStream might fail at that point, but number of desired channels is already set
	// so mark that setup finished, upper layer shall check for errors and decide what to do next
	setupFinished = true;
}

void PortAudioSystem::configureStream() {
	if (verbose) {
		Log::d() << "PortAudioSystem::configureStream()";
	}

	numInChannels  = isInPortDummy ? 0 : desiredNumInChannels;
	numOutChannels = desiredNumOutChannels;

	// just make sure our ports are up to date
	rescanPorts();
	PaStreamParameters inputParameters, outputParameters;

	if (this->numInChannels > 0) {
		if (!inPort.isValid()) {
			inPort		  = getPort(Pa_GetDefaultInputDevice());
			isInPortDummy = false;
		}
		inputParameters.device = inPort.portId;
		if (inputParameters.device == paNoDevice) {
			Log::e() << "Error: No default input device.";
			numInChannels = 0;
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
			outPort = getPort(Pa_GetDefaultOutputDevice());
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
	auto err = Pa_StartStream(stream);
	checkPaError(err, "start stream");
	Log::d() << to_string(getOutputLatency() * 1000.0, 0)
			 << "ms output latency, buffersize: " << std::to_string(bufferSize) << " samples";
}

void PortAudioSystem::stop() {
	auto err = Pa_StopStream(stream);
	checkPaError(err, "stop stream");
}

bool PortAudioSystem::isRunning() {
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

void printOutPorts();
void PortAudioSystem::rescanPorts() {
#ifdef __APPLE__
	printOutPorts();
#endif
	if (verbose) {
		Log::d() << "PortAudioSystem::rescanPorts()";
	}
	ports.clear();

	int defaultInputDeviceId  = Pa_GetDefaultInputDevice();
	int defaultOutputDeviceId = Pa_GetDefaultOutputDevice();

	int numDevices = Pa_GetDeviceCount();
	if (numDevices < 0) {
		Log::e() << "Couldn't get number of devices from PortAudio! - count was " << numDevices;
		return;
	}
	if (verbose) {
		Log::d() << "Portaudio: Found " << numDevices << " devices";
	}
	//	Log::d() << "Found " << numDevices << " devices";
	const vector<double> standardSampleRates = {

		//8000.0, 9600.0, 11025.0, 12000.0, 16000.0, 22050.0, 24000.0, 32000.0,

		44100.0,
		48000.0,

		//88200.0, 96000.0, 192000.0,
	};

	for (int i = 0; i < numDevices; i++) {
		auto dev = Pa_GetDeviceInfo(i);

		AudioPort port;
		port.portId			= i;
		port.name			= dev->name;
		port.numInChannels	= dev->maxInputChannels;
		port.numOutChannels = dev->maxOutputChannels;

		//		Log::d() << "Found port " << port.name << " with " << port.numInChannels << " in channels and "
		//				 << port.numOutChannels << " out channels";

		if (i == defaultInputDeviceId) {
			port.isDefaultInput = true;
		}
		if (i == defaultOutputDeviceId) {
			port.isDefaultOutput = true;
		}

		port.defaultSampleRate = dev->defaultSampleRate;

		/*
        for(const auto sr : standardSampleRates) {
            PaStreamParameters inputParameters, outputParameters;
            inputParameters.device = i;
            inputParameters.channelCount = dev->maxInputChannels;
            inputParameters.sampleFormat = paFloat32;
            inputParameters.suggestedLatency = 0; // ignored by Pa_IsFormatSupported()
            inputParameters.hostApiSpecificStreamInfo = NULL;

            outputParameters.device = i;
            outputParameters.channelCount = dev->maxOutputChannels;
            outputParameters.sampleFormat = paFloat32;
            outputParameters.suggestedLatency = 0; // ignored by Pa_IsFormatSupported()
            outputParameters.hostApiSpecificStreamInfo = NULL;
            auto *ins = &inputParameters;
            auto *outs = &outputParameters;
            if(ins->channelCount==0) ins = nullptr;
            if(outs->channelCount==0) outs = nullptr;
            auto err = Pa_IsFormatSupported( ins, outs, sr );
            if( err == paFormatIsSupported ) {
                port.supportedSampleRates.push_back(sr);
            } else {
                Log::e() << "Got error in rescanPorts() " << Pa_GetErrorText(err);
            }
        }
        */

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

/////////////////////////////////////
#ifdef __APPLE__
#	include <AudioToolbox/AudioToolbox.h>
#	include <CoreAudio/CoreAudio.h>
#	include <CoreFoundation/CoreFoundation.h>
//#include <iostream>
//#include <string>
//#include <vector>
//#include <iomanip>
//#include <sstream>
std::vector<AudioPort> rescan_Ports() {
	std::vector<AudioPort> ports;

	AudioDeviceID defaultInput	= 0;
	AudioDeviceID defaultOutput = 0;
	UInt32 size					= sizeof(AudioDeviceID);

	// Get default input/output
	AudioObjectPropertyAddress defInputAddr = {kAudioHardwarePropertyDefaultInputDevice,
											   kAudioObjectPropertyScopeGlobal,
											   kAudioObjectPropertyElementMain};
	AudioObjectGetPropertyData(kAudioObjectSystemObject, &defInputAddr, 0, nullptr, &size, &defaultInput);

	AudioObjectPropertyAddress defOutputAddr = {kAudioHardwarePropertyDefaultOutputDevice,
												kAudioObjectPropertyScopeGlobal,
												kAudioObjectPropertyElementMain};
	AudioObjectGetPropertyData(kAudioObjectSystemObject, &defOutputAddr, 0, nullptr, &size, &defaultOutput);

	// Get all audio devices
	AudioObjectPropertyAddress devicesAddr = {
		kAudioHardwarePropertyDevices, kAudioObjectPropertyScopeGlobal, kAudioObjectPropertyElementMain};
	AudioObjectGetPropertyDataSize(kAudioObjectSystemObject, &devicesAddr, 0, nullptr, &size);
	UInt32 deviceCount		 = size / sizeof(AudioDeviceID);
	AudioDeviceID *deviceIDs = new AudioDeviceID[deviceCount];
	AudioObjectGetPropertyData(kAudioObjectSystemObject, &devicesAddr, 0, nullptr, &size, deviceIDs);

	for (UInt32 i = 0; i < deviceCount; i++) {
		AudioDeviceID deviceID = deviceIDs[i];
		AudioPort port;
		port.portId = static_cast<int>(deviceID);

		// Device name
		CFStringRef deviceName				= nullptr;
		AudioObjectPropertyAddress nameAddr = {
			kAudioObjectPropertyName, kAudioObjectPropertyScopeGlobal, kAudioObjectPropertyElementMain};
		size = sizeof(deviceName);
		if (AudioObjectGetPropertyData(deviceID, &nameAddr, 0, nullptr, &size, &deviceName) == noErr) {
			char name[256];
			CFStringGetCString(deviceName, name, sizeof(name), kCFStringEncodingUTF8);
			port.name = name;
			CFRelease(deviceName);
		}

		// Default sample rate
		AudioObjectPropertyAddress rateAddr = {kAudioDevicePropertyNominalSampleRate,
											   kAudioObjectPropertyScopeGlobal,
											   kAudioObjectPropertyElementMain};
		size								= sizeof(Float64);
		Float64 defaultRate;
		if (AudioObjectGetPropertyData(deviceID, &rateAddr, 0, nullptr, &size, &defaultRate) == noErr) {
			port.defaultSampleRate = defaultRate;
		}

		// Supported sample rates
		AudioObjectPropertyAddress availableRatesAddr = {kAudioDevicePropertyAvailableNominalSampleRates,
														 kAudioObjectPropertyScopeGlobal,
														 kAudioObjectPropertyElementMain};
		size										  = 0;
		if (AudioObjectGetPropertyDataSize(deviceID, &availableRatesAddr, 0, nullptr, &size) == noErr) {
			UInt32 count			= size / sizeof(AudioValueRange);
			AudioValueRange *ranges = new AudioValueRange[count];
			if (AudioObjectGetPropertyData(deviceID, &availableRatesAddr, 0, nullptr, &size, ranges) == noErr) {
				for (UInt32 j = 0; j < count; j++) {
					if (ranges[j].mMinimum == ranges[j].mMaximum) {
						port.supportedSampleRates.push_back(ranges[j].mMinimum);
					} else {
						// continuous range; just log min & max
						port.supportedSampleRates.push_back(ranges[j].mMinimum);
						port.supportedSampleRates.push_back(ranges[j].mMaximum);
					}
				}
			}
			delete[] ranges;
		}

		// Input channels
		AudioObjectPropertyAddress inAddr = {kAudioDevicePropertyStreamConfiguration,
											 kAudioDevicePropertyScopeInput,
											 kAudioObjectPropertyElementMain};
		size							  = 0;
		if (AudioObjectGetPropertyDataSize(deviceID, &inAddr, 0, nullptr, &size) == noErr) {
			AudioBufferList *bufferList = (AudioBufferList *) malloc(size);
			if (AudioObjectGetPropertyData(deviceID, &inAddr, 0, nullptr, &size, bufferList) == noErr) {
				for (UInt32 j = 0; j < bufferList->mNumberBuffers; j++) {
					port.numInChannels += bufferList->mBuffers[j].mNumberChannels;
				}
			}
			free(bufferList);
		}

		// Output channels
		AudioObjectPropertyAddress outAddr = {kAudioDevicePropertyStreamConfiguration,
											  kAudioDevicePropertyScopeOutput,
											  kAudioObjectPropertyElementMain};
		size							   = 0;
		if (AudioObjectGetPropertyDataSize(deviceID, &outAddr, 0, nullptr, &size) == noErr) {
			AudioBufferList *bufferList = (AudioBufferList *) malloc(size);
			if (AudioObjectGetPropertyData(deviceID, &outAddr, 0, nullptr, &size, bufferList) == noErr) {
				for (UInt32 j = 0; j < bufferList->mNumberBuffers; j++) {
					port.numOutChannels += bufferList->mBuffers[j].mNumberChannels;
				}
			}
			free(bufferList);
		}

		// Default input/output check
		if (deviceID == defaultInput) port.isDefaultInput = true;
		if (deviceID == defaultOutput) port.isDefaultOutput = true;

		ports.push_back(port);
	}

	delete[] deviceIDs;
	return ports;
}
void printOutPorts() {
	auto ports = rescan_Ports();
	// Print all ports
	for (const auto &port: ports) {
		Log::d() << port.toString();
	}
}
#endif