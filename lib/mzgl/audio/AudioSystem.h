//
//  MZOpenGLView.h
//  MZGL
//
//  Created by Marek Bereza on 15/01/2018.
//  Copyright © 2018 Marek Bereza. All rights reserved.
//
#pragma once

#include <functional>
#include <string>
#include <vector>
#include "stringUtil.h"

struct AudioPort {
	int portId		   = -1;
	int numInChannels  = 0;
	int numOutChannels = 0;
	double defaultSampleRate;
	std::vector<double> supportedSampleRates;
	std::string name;
	bool isDefaultInput	 = false;
	bool isDefaultOutput = false;

	bool isValid() { return portId != -1; }

	std::string toString() {
		std::string s = "[id: " + std::to_string(portId) + "] " + name + " (ins: " + std::to_string(numInChannels);
		s += " / outs: " + std::to_string(numOutChannels);
		if (isDefaultInput) s += ",default-input";
		if (isDefaultOutput) s += ",default-output";
		s += ") - default sr: " + to_string(defaultSampleRate, 1) + " other: ";
		for (int i = 0; i < supportedSampleRates.size(); i++) {
			s += to_string(supportedSampleRates[i], 1) + " ";
		}

		return s;
	}
};

class SampleRateChangeListener {
public:
	virtual void sampleRateChanged(double newSampleRate) = 0;
};

class AudioIO {
public:
	virtual void audioIn(float *data, int frames, int chans) {}
	virtual void audioOut(float *data, int frames, int chans) {}
};

enum class StreamConfigurationStatus {
	OK,
	FAILED, // TODO: more descriptive errors can be added here
};

class _AudioSystem {
public:
	void bindToApp(AudioIO *app);
	virtual ~_AudioSystem() {}
	virtual void setup(int numInChannels, int numOutChannels) = 0;
	virtual void start()									  = 0;
	virtual void stop()										  = 0;
	virtual bool isRunning()								  = 0;

	int numInChannels  = 2;
	int numOutChannels = 2;
	virtual void setVerbose(bool) {}
	std::function<void(float *, int, int)> inputCallback  = [](float *, int, int) {};
	std::function<void(float *, int, int)> outputCallback = [](float *, int, int) {};

	void printPorts();
	virtual void rescanPorts() {}

	virtual std::vector<AudioPort> getInputs()	= 0;
	virtual std::vector<AudioPort> getOutputs() = 0;

	virtual bool setInput(const AudioPort &audioInput);
	virtual bool setOutput(const AudioPort &audioOutput);

	bool setInputByName(const std::string &name);
	bool setOutputByName(const std::string &name);

	virtual AudioPort getInput();
	virtual AudioPort getOutput();

	virtual double getOutputLatency();

	// this should restart the system and try to set the samplerate
	virtual void setSampleRate(float sampleRate);
	virtual void setBufferSize(int size);

	virtual float getSampleRate() const { return sampleRate; }
	virtual int getBufferSize() const { return bufferSize; }
	void bufferSizeChangedBySystem(int size);

	void addSampleRateChangeListener(SampleRateChangeListener *listener);
	void removeSampleRateChangeListener(SampleRateChangeListener *listener);
	void notifySampleRateChanged();

	[[nodiscard]] virtual double getNanoSecondsAtBufferBegin() { return 0; }

	StreamConfigurationStatus getStreamConfigurationStatus() { return streamConfigStatus_; }

protected:
	std::vector<SampleRateChangeListener *> listeners;
	uint32_t bufferSize							  = 256;
	float sampleRate							  = 0;
	StreamConfigurationStatus streamConfigStatus_ = StreamConfigurationStatus::OK;
};

class DummyAudioSystem : public _AudioSystem {
public:
	DummyAudioSystem()
		: _AudioSystem() {
		sampleRate = 48000.f;
	}
	void setSampleRate(float sampleRate) override { this->sampleRate = sampleRate; }
	void setup(int numInChannels, int numOutChannels) override {}
	void start() override {}
	void stop() override {}
	bool isRunning() override { return false; }

	void setVerbose(bool v) override {}

	std::vector<AudioPort> getInputs() override { return {}; }
	std::vector<AudioPort> getOutputs() override { return {}; }
};

#ifdef __APPLE__
#	include <TargetConditionals.h>
#endif

#if TARGET_OS_IOS
#	include <AudioToolbox/AudioToolbox.h>
#	include "AudioSystemIOS.h"
#else
#	ifndef __ANDROID__
#		include "PortAudioSystem.h"
#	endif
#endif
