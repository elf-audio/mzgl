//
//  MZOpenGLView.h
//  MZGL
//
//  Created by Marek Bereza on 15/01/2018.
//  Copyright © 2018 Marek Bereza. All rights reserved.
//

#pragma once
#include "AudioSystem.h"


typedef void PaStream;
typedef int PaError;


class PortAudioSystem : public _AudioSystem {
public:
	PortAudioSystem();
	virtual ~PortAudioSystem() override;
	void setup(int numInChannels, int numOutChannels) override;
	void start() override;
	void stop() override;
	bool isRunning() override;
	
	void setVerbose(bool verbose) override { this->verbose = verbose; }
	std::vector<AudioPort> getInputs() override;
	std::vector<AudioPort> getOutputs() override;

	bool setInput(const AudioPort &audioInput) override;
	bool setOutput(const AudioPort &audioOutput) override;
	void rescanPorts() override;
	
	AudioPort getInput() override { return inPort; }
	AudioPort getOutput() override { return outPort; }

    double getLatency();
    double getOutputLatency() override;
    double getTimeAtBufferBegin() override;
    double getHostTime();
    
    
    // The time when the first sample of the input buffer was captured at the ADC input
    double inputTime = 0;
    
    // The time when the first sample of the output buffer will output the DAC
    double outputTime = 0; // this is host time + latency I think.
    
private:
    
    uint64_t hostTime = 0;
	bool isSetup = false;
    AudioPort inPort;
    AudioPort outPort;

    void configureStream();

	bool verbose = false;
	bool running = false;
	bool checkPaError(PaError err, std::string msg);
//	void printDevices();
	PaStream *stream = nullptr;
	
	std::vector<AudioPort> ports;
	
	// don't hold onto this port for too long
	AudioPort getPort(int dev);
	
private:
	int desiredNumInChannels = 2;
	int desiredNumOutChannels = 2;
};

typedef PortAudioSystem AudioSystem;
//typedef DummyAudioSystem AudioSystem;
//#ifdef __APPLE__
//double getMacDefaultDeviceSampleRate();
//#endif


