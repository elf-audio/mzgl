//
//  AudioSystemRtAudio.cpp
//  MZGL
//
//  Created by Marek Bereza on 23/01/2018.
//  Copyright © 2018 Marek Bereza. All rights reserved.
//

#include "AudioSystemRtAudio.h"
#include "RtAudio.h"
#include "log.h"
#ifdef __APPLE__
#	import <AVFoundation/AVFoundation.h>
#endif

using namespace std;

AudioSystemRtAudio::AudioSystemRtAudio() {
	audio = std::shared_ptr<RtAudio>(new RtAudio());
}
AudioSystemRtAudio::~AudioSystemRtAudio() {
	if (isRunning()) {
		stop();
	}
	if (audio->isStreamOpen()) audio->closeStream();
}
//
//void AudioSystemRtAudio::printDevices() {
//	// Determine the number of devices available
//	unsigned int devices = audio->getDeviceCount();
//	// Scan through devices for various capabilities
//	RtAudio::DeviceInfo info;
//	for ( unsigned int i=0; i<devices; i++ ) {
//		info = audio->getDeviceInfo( i );
//		if ( info.probed == true ) {
//			// Print, for example, the maximum number of output channels for each device
//			std::cout << "device = \""  << info.name << "\" (" << i<<")";
//			std::cout << ": out channels = " << info.outputChannels << "\n";
//		}
//	}
//}

void AudioSystemRtAudio::startAudioCallback() {
	inProcess.store(true);
}

void AudioSystemRtAudio::finishedAudioCallback() {
	inProcess.store(false);
}

int AudioSystemRtAudio_callback(void *outputBuffer,
								void *inputBuffer,
								unsigned int nBufferFrames,
								double streamTime,
								RtAudioStreamStatus status,
								void *data) {
	// Since the number of input and output channels is equal, we can do
	// a simple buffer copy operation here.
	if (status) std::cout << "Stream over/underflow detected." << std::endl;
	AudioSystemRtAudio *audio = (AudioSystemRtAudio *) data;
	audio->startProcessCall();
	if (inputBuffer != nullptr) {
		audio->inputCallback((float *) inputBuffer, nBufferFrames, audio->numInChannels);
	}
	if (outputBuffer != nullptr) {
		audio->outputCallback((float *) outputBuffer, nBufferFrames, audio->numOutChannels);
	}
	audio->finishedProcessCall();
	return 0;
}

#ifdef __APPLE__
double getMacDefaultDeviceSampleRate() {
	AudioObjectID deviceID;

	// load the current default device
	UInt32 deviceSize				   = sizeof(deviceID);
	AudioObjectPropertyAddress address = {kAudioHardwarePropertyDefaultInputDevice,
										  kAudioObjectPropertyScopeGlobal,
										  kAudioObjectPropertyElementMaster};

	auto err = AudioObjectGetPropertyData(kAudioObjectSystemObject, &address, 0, NULL, &deviceSize, &deviceID);

	if (err != kAudioHardwareNoError) {
		NSLog(@"Error getting default device");
		return 0;
	}

	AudioObjectPropertyAddress addr;

	addr.mSelector = kAudioDevicePropertyNominalSampleRate;
	addr.mScope	   = kAudioObjectPropertyScopeGlobal;
	addr.mElement  = kAudioObjectPropertyElementMaster;

	UInt32 dataSize = 0;
	err				= AudioObjectGetPropertyDataSize(deviceID, &addr, 0, NULL, &dataSize);

	if (err != kAudioHardwareNoError) {
		NSLog(@"Error getting prop size");
		return 0;
	}

	double val;

	err = AudioObjectGetPropertyData(deviceID, &addr, 0, NULL, &dataSize, &val);

	if (err != kAudioHardwareNoError) {
		NSLog(@"Error getting prop");
		return 0;
	}
	//	this->sampleRate = val;
	//					this->sampleRate = outSampleRate;
	//			NSLog(@"Want %f Hz", this->sampleRate);

	return val;
}
#endif

// TODO: can't re-setup audio in AudioSystemRtAudio
void AudioSystemRtAudio::setup(int numInChannels, int numOutChannels) {
	this->numInChannels	 = numInChannels;
	this->numOutChannels = numOutChannels;

	//	printDevices();

	RtAudio::StreamParameters *iParams = nullptr;
	RtAudio::StreamParameters *oParams = nullptr;

	if (this->numInChannels > 0) {
		iParams			   = new RtAudio::StreamParameters();
		iParams->deviceId  = audio->getDefaultInputDevice();
		iParams->nChannels = this->numInChannels;
		auto dev		   = audio->getDeviceInfo(audio->getDefaultInputDevice());
		if (iParams->nChannels != dev.inputChannels) {
			Log::e() << "Default input device '" << dev.name << "' doesn't have " << iParams->nChannels
					 << " - going with " << dev.inputChannels;
			this->numInChannels = iParams->nChannels = dev.inputChannels;
		}
		Log::v() << dev.name;
	}

	if (this->numOutChannels > 0) {
		oParams			   = new RtAudio::StreamParameters();
		oParams->deviceId  = audio->getDefaultOutputDevice();
		oParams->nChannels = this->numOutChannels;
	}
	try {
#ifdef __APPLE__
		// if on mac, look up the default sample rate if we're on the default audio device
		if (audio->getDeviceInfo(oParams->deviceId).isDefaultOutput && this->sampleRate == 0) {
			this->sampleRate = getMacDefaultDeviceSampleRate();
			this->sampleRate = 48000;
		}

#endif
		auto oDevInfo = audio->getDeviceInfo(oParams->deviceId);
		if (this->sampleRate == 0) {
			Log::d() << "Setting to preferred sample rate " << this->sampleRate;
			this->sampleRate = oDevInfo.preferredSampleRate;
		}
#ifdef _WIN32
		sampleRate = 44100;
#endif
		RtAudio::StreamOptions opts;
		opts.flags = RTAUDIO_MINIMIZE_LATENCY;
		audio->openStream(oParams,
						  iParams,
						  RTAUDIO_FLOAT32,
						  sampleRate,
						  &bufferSize,
						  &AudioSystemRtAudio_callback,
						  (void *) this,
						  &opts);

		this->sampleRate = audio->getStreamSampleRate();
	} catch (RtAudioError &e) {
		e.printMessage();
	}
	if (iParams != nullptr) delete iParams;
	if (oParams != nullptr) delete oParams;
}

void AudioSystemRtAudio::start() {
	try {
		audio->startStream();
		running.store(true);
	} catch (RtAudioError &e) {
		e.printMessage();
	}
}

void AudioSystemRtAudio::stop() {
	try {
		audio->stopStream();
		running.store(false);
	} catch (RtAudioError &e) {
		e.printMessage();
	}
}

bool AudioSystemRtAudio::isRunning() {
	return running.load();
}

bool AudioSystemRtAudio::audioThreadIsStopped() {
	return !isRunning() && !inProcess.load();
}

AudioPort createAudioPortFromRTAudio(const RtAudio::DeviceInfo &info, int devId) {
	AudioPort ap;
	ap.portId = devId;
	ap.name	  = info.name;

	ap.numOutChannels  = info.outputChannels;
	ap.isDefaultOutput = info.isDefaultOutput;

	ap.numInChannels  = info.inputChannels;
	ap.isDefaultInput = info.isDefaultInput;

	return ap;
}

vector<AudioPort> AudioSystemRtAudio::getOutputs() {
	vector<AudioPort> ports;

	for (unsigned int i = 0; i < audio->getDeviceCount(); i++) {
		auto info = audio->getDeviceInfo(i);

		if (info.probed && info.outputChannels > 0) {
			ports.push_back(createAudioPortFromRTAudio(info, i));
		}
	}
	return ports;
}

vector<AudioPort> AudioSystemRtAudio::getInputs() {
	vector<AudioPort> ports;

	for (unsigned int i = 0; i < audio->getDeviceCount(); i++) {
		auto info = audio->getDeviceInfo(i);
		if (info.probed && info.inputChannels > 0) {
			ports.push_back(createAudioPortFromRTAudio(info, i));
		}
	}
	return ports;
}
