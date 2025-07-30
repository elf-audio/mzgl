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
#include "Listenable.h"

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

	std::string toString() const {
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
	virtual ~SampleRateChangeListener() {}
	virtual void sampleRateChanged(double newSampleRate) = 0;
};

class AudioIO {
public:
	virtual ~AudioIO() {}
	virtual void audioIn(float *data, int frames, int chans) {}
	virtual void audioOut(float *data, int frames, int chans) {}
};

enum class StreamConfigurationStatus {
	OK,
	FAILED, // TODO: more descriptive errors can be added here
};

class AudioDeviceChangeListener {
public:
	virtual ~AudioDeviceChangeListener() {}
	virtual void audioDeviceChanged() = 0;
};

class _AudioSystem {
public:
	Listenable<AudioDeviceChangeListener> deviceChanges;

	void bindToApp(AudioIO *app);
	virtual ~_AudioSystem() {}
	virtual void setup(int numInChannels, int numOutChannels) = 0;
	virtual void start()									  = 0;
	virtual void stop()										  = 0;
	[[nodiscard]] virtual bool isRunning()					  = 0;
	[[nodiscard]] virtual bool isInsideAudioCallback()		  = 0;

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

	[[nodiscard]] virtual double getNanoSecondsAtBufferBegin() { return 0; }

	std::vector<std::string> getInputDeviceNames();
	std::vector<std::string> getOutputDeviceNames();

	StreamConfigurationStatus getStreamConfigurationStatus() { return streamConfigStatus_; }

protected:
	void notifySampleRateChanged();

	uint32_t bufferSize							  = 256;
	double sampleRate							  = 48'000;
	StreamConfigurationStatus streamConfigStatus_ = StreamConfigurationStatus::OK;

private:
	std::vector<SampleRateChangeListener *> sampleRateChangeListeners;
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
	[[nodiscard]] bool isRunning() override { return false; }
	[[nodiscard]] bool isInsideAudioCallback() override { return false; }

	void setVerbose(bool v) override {}

	std::vector<AudioPort> getInputs() override { return {}; }
	std::vector<AudioPort> getOutputs() override { return {}; }
};

#include "mzgl_platform.h"

#if MZGL_IOS
#	include "AudioSystemIOS.h"
#elif !MZGL_ANDROID
#	include "PortAudioSystem.h"
#endif
