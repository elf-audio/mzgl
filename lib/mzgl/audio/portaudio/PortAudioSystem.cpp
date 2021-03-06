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
#   include "pa_linux_alsa.h"
#endif

using namespace std;

bool PortAudioSystem::checkPaError(PaError err, string msg) {
	if(err != paNoError) {
		Log::e() << "PortAudio error: " << msg << " - error is " << err << "("<<Pa_GetErrorText(err)<<")";
		return false;
	} else if(verbose) {
		Log::d() << "PortAudioSystem Success " << msg;
	}
	
	return true;
}

PortAudioSystem::PortAudioSystem() {
	if(verbose) {
		Log::d() << "PortAudioSystem()";
		printf("PortAudioSystem()\n");
	}
	auto err = Pa_Initialize();
	if(!checkPaError(err, "Intializing port audio")) {
		throw std::runtime_error("dang! portaudio not working");
	}
	rescanPorts();
}

PortAudioSystem::~PortAudioSystem() {
	if(verbose) {
		Log::e() << "Tearing down PortAudioSystem";
	}
	if(isRunning()) {
		stop();
	}
	if(stream!=nullptr && !Pa_IsStreamStopped(stream)) {
		auto err = Pa_StopStream(stream);
		checkPaError(err, "stopping");
	}
	auto err = Pa_Terminate();
	checkPaError(err, "terminating");
}

double PortAudioSystem::getTimeAtBufferBegin() {
    return outputTime;
}

static int PortAudioSystem_callback( const void *inputBuffer, void *outputBuffer,
						 unsigned long framesPerBuffer,
						 const PaStreamCallbackTimeInfo* timeInfo,
						 PaStreamCallbackFlags statusFlags,
						 void *userData )
{

    PortAudioSystem *as = (PortAudioSystem*)userData;
	
    as->inputTime = timeInfo->inputBufferAdcTime;  /**< The time when the first sample of the input buffer was captured at the ADC input */
    as->outputTime = timeInfo->outputBufferDacTime; 
    
	if(inputBuffer!=nullptr) {
		as->inputCallback((float*)inputBuffer, (int)framesPerBuffer, as->numInChannels);
	}

	if(outputBuffer!=nullptr) {
		as->outputCallback((float*) outputBuffer, (int)framesPerBuffer, as->numOutChannels);
	}

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
	if(verbose) {
		Log::d() << "Gettting port for " << dev;
	}
	for(auto &p : ports) {
		if(p.portId==dev) {
			return p;
		}
	}
	return AudioPort();
}

bool PortAudioSystem::setInput(const AudioPort &audioInput) {
	if(verbose) {
		Log::d() << "Setting input to " << audioInput.name;
	}
    bool shouldStopAndStart = isRunning();
    if(shouldStopAndStart) stop();
    inPort = audioInput;
	if(isSetup) configureStream();
    if(shouldStopAndStart) start();

    return true;
}
bool PortAudioSystem::setOutput(const AudioPort &audioOutput) {
	if(verbose) {
		Log::d() << "Setting output to " << audioOutput.name;
	}
    bool shouldStopAndStart = isRunning();
    if(shouldStopAndStart) stop();
    outPort = audioOutput;
    if(isSetup) configureStream();
    if(shouldStopAndStart) start();
	return true;
}


void PortAudioSystem::setup(int numIns, int numOuts) {
	if(verbose) {
		Log::d() << "PortAudioSystem::setup("<<numIns << ", "<< numOuts<<")";
	}
	this->numInChannels = numIns;
	this->numOutChannels = numOuts;
	this->desiredNumInChannels = numIns;
	this->desiredNumOutChannels = numOuts;
	
	configureStream();
	isSetup = true;
}

void PortAudioSystem::configureStream() {
	if(verbose) {
		Log::d() << "PortAudioSystem::configureStream()";
	}
	numInChannels = desiredNumInChannels;
	numOutChannels = desiredNumOutChannels;
    // just make sure our ports are up to date
    rescanPorts();
    PaStreamParameters inputParameters, outputParameters;

    if(this->numInChannels>0) {
        if(!inPort.isValid()) {
            inPort = getPort(Pa_GetDefaultInputDevice());
        }
        inputParameters.device = inPort.portId;
        if (inputParameters.device == paNoDevice) {
            Log::e() << "Error: No default input device.";
            numInChannels = 0;
            //return;
        } else {

            numInChannels = min(inPort.numInChannels, numInChannels);
            inputParameters.channelCount = numInChannels;
            inputParameters.sampleFormat = paFloat32;
            inputParameters.suggestedLatency = Pa_GetDeviceInfo(inputParameters.device)->defaultLowInputLatency;
            inputParameters.hostApiSpecificStreamInfo = nullptr;
        }
    }

    if(this->numOutChannels>0) {
        if(!outPort.isValid()) {
            outPort = getPort(Pa_GetDefaultOutputDevice());
        }

        outputParameters.device = outPort.portId;
        if (outputParameters.device == paNoDevice) {
            Log::e() << "Error: No default output device.";
            return;
        }
        numOutChannels = min((int)outPort.numOutChannels, numOutChannels);
        outputParameters.channelCount = numOutChannels;

        outputParameters.sampleFormat = paFloat32;
        outputParameters.suggestedLatency = Pa_GetDeviceInfo( outputParameters.device )->defaultLowOutputLatency;
        outputParameters.hostApiSpecificStreamInfo = nullptr;
    }

    if(sampleRate==0) {
        if(this->numOutChannels>0) {
            sampleRate = outPort.defaultSampleRate;
//            Log::d() << "Using default output sample rate of " << outPort.defaultSampleRate;
        } else if(this->numInChannels>0) {
            sampleRate = inPort.defaultSampleRate;
        }
    } else {
        Log::d() << "Using user specified sample rate of " << sampleRate;
    }
    if(verbose) {
		Log::d() << "---------------------------------------------------------";
        for(auto & p : ports) {
            Log::d() << "PORT: " << p.toString();
        }
    }


#ifdef __linux__
    PaAlsa_EnableRealtimeScheduling(&stream, true);
#endif

    auto *inParams = &inputParameters;
    auto *outParams = &outputParameters;

    if(numInChannels<1) inParams = nullptr;
    if(numOutChannels<1) outParams = nullptr;

	if(verbose) {
		Log::d() << "Calling Pa_OpenStream";
	}
	
    auto err = Pa_OpenStream(
            &stream,
            inParams,
            outParams,
            sampleRate,
            bufferSize,
            0, /* paClipOff, */  /* we won't output out of range samples so don't bother clipping them */
            PortAudioSystem_callback,
            this );
    if(!checkPaError(err, "open stream")) {
        return;
	} else if(verbose) {
		Log::d() << "Success opening stream";
	}

}
void PortAudioSystem::start() {
	auto err = Pa_StartStream(stream);
	checkPaError(err, "start stream");
	Log::d() << to_string(getOutputLatency()*1000.0, 0) << "ms output latency, buffersize: " << std::to_string(bufferSize) << "samples";
}

void PortAudioSystem::stop() {
	auto err = Pa_StopStream( stream );
	checkPaError(err, "stop stream");
}

bool PortAudioSystem::isRunning() {
	if(verbose) {
		Log::d() << "PortAudioSystem::isRunning()" << Pa_IsStreamActive(stream);
	}
	return Pa_IsStreamActive(stream)==1;
}


vector<AudioPort> PortAudioSystem::getOutputs() {
//	updatePorts();
	vector<AudioPort> ret;
	for(const auto &p : ports) {
		if(p.numOutChannels>0) ret.push_back(p);
	}
	return ret;
}


vector<AudioPort> PortAudioSystem::getInputs() {
//	updatePorts();
	vector<AudioPort> ret;
	for(const auto &p : ports) {
		if(p.numInChannels>0) ret.push_back(p);
	}
	return ret;
}

void PortAudioSystem::rescanPorts() {
	if(verbose) {
		Log::d() << "PortAudioSystem::rescanPorts()";
	}
	ports.clear();
	
	int defaultInputDeviceId = Pa_GetDefaultInputDevice();
	int defaultOutputDeviceId = Pa_GetDefaultOutputDevice();
	
	int numDevices = Pa_GetDeviceCount();
	if(numDevices<0) {
		Log::e() << "Couldn't get number of devices from PortAudio! - count was " << numDevices;
		return;
	}
	
	const vector<double> standardSampleRates = {
	
		//8000.0, 9600.0, 11025.0, 12000.0, 16000.0, 22050.0, 24000.0, 32000.0,
		
		44100.0, 48000.0, 
		
		
		//88200.0, 96000.0, 192000.0,
	};

	for(int i = 0; i < numDevices; i++) {
		auto dev = Pa_GetDeviceInfo(i);

		AudioPort port;
		port.portId = i;
		port.name = dev->name;
		port.numInChannels = dev->maxInputChannels;
		port.numOutChannels = dev->maxOutputChannels;
		if(i==defaultInputDeviceId) {
			port.isDefaultInput = true;
		}
		if(i==defaultOutputDeviceId) {
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
    const auto *info = Pa_GetStreamInfo( stream );
    return info->inputLatency + info->outputLatency;
}

double PortAudioSystem::getOutputLatency() {
    const auto *info = Pa_GetStreamInfo( stream );
    return info->outputLatency;
}

double PortAudioSystem::getHostTime() {
    return hostTime;
}
