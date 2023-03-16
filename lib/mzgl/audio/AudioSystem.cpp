//
//  AudioSystem.cpp
//  MZGL
//
//  Created by Marek Bereza on 23/01/2018.
//  Copyright © 2018 Marek Bereza. All rights reserved.
//

#include "AudioSystem.h"
#include "log.h"
void _AudioSystem::bindToApp(AudioIO *app) {

	outputCallback = [app](float* a, int b, int c) {
		app->audioOut(a, b, c);
	};
	
	inputCallback = [app](float* a, int b, int c) {
		app->audioIn(a, b, c);
	};
}
	
void _AudioSystem::addSampleRateChangeListener(SampleRateChangeListener *listener) {
	listeners.push_back(listener);
}
void _AudioSystem::removeSampleRateChangeListener(SampleRateChangeListener *listener) {
	for(int i =0 ; i < listeners.size(); i++) {
		if(listeners[i]==listener) {
			listeners.erase(listeners.begin() + i);
			return;
		}
	}
//	Log::e() << "Error - couldn't find sample rate listener to erase";
}

bool _AudioSystem::setInput(const AudioPort &audioInput) {
    Log::e() << "AudioSystem::setInput() not implemented - you got to do it yourself";
    return false;
}

bool _AudioSystem::setOutput(const AudioPort &audioOutput) {
    Log::e() << "AudioSystem::setOutput() not implemented - you got to do it yourself";
    return false;
}

AudioPort _AudioSystem::getInput() {
    Log::e() << "AudioSystem::getInput() not implemented - you got to do it yourself";
    return AudioPort();
}

AudioPort _AudioSystem::getOutput() {
    Log::e() << "AudioSystem::getOutput() not implemented - you got to do it yourself";
    return AudioPort();
}

void _AudioSystem::notifySampleRateChanged() {
	for(auto *l : listeners) {
		l->sampleRateChanged(sampleRate);
	}
}

void _AudioSystem::setSampleRate(float sampleRate) {
	bool wasRunning = isRunning();
	if(wasRunning) stop();
	this->sampleRate = sampleRate;
	if(wasRunning) start();
}

void _AudioSystem::setBufferSize(int size) {
	bool wasRunning = isRunning();
	if(wasRunning) stop();
	this->bufferSize = size;
	if(wasRunning) start();
}

void _AudioSystem::bufferSizeChangedBySystem(int size) {
	this->bufferSize = size;
}

void _AudioSystem::printPorts() {
	
	printf("======================================\n");
	printf("INPUTS\n");
	auto a = getInputs();
	for(auto &inp : a) {
		printf("%s\n", inp.toString().c_str());
	}
	printf("--------------------------------------\n");
	printf("OUTPUTS: \n");
	a = getOutputs();
	for(auto &oup : a) {
		printf("%s\n", oup.toString().c_str());
	}
	printf("======================================\n");
}

bool _AudioSystem::setInputByName(const std::string &name) {
	auto ports = getInputs();
	for(auto &p : ports) {
		if(p.name==name) {
			return setInput(p);
		}
	}
	return false;
}

bool _AudioSystem::setOutputByName(const std::string &name) {
	auto ports = getOutputs();
	for(auto &p : ports) {
		if(p.name==name) {
			return setOutput(p);
		}
	}
	return false;
}
double _AudioSystem::getOutputLatency() {
    Log::e() << "Error: AudioSystem::getLatency() not implemented!";
    return 0.0;
}
