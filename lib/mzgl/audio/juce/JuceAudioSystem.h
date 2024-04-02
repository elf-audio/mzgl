//
// Created by Marek Bereza on 26/03/2024.
//

#pragma once
#include <memory>
#include "AudioSystem.h"
class JuceImpl;
class JuceAudioSystem : public _AudioSystem {
public:
	JuceAudioSystem();
	virtual ~JuceAudioSystem() {}
	void setup(int numInChannels, int numOutChannels) override;
	void start() override;
	void stop() override;
	bool isRunning() override;

	float getSampleRate() const override;
	int getBufferSize() const override;

	std::vector<AudioPort> getInputs() override;
	std::vector<AudioPort> getOutputs() override;

private:
	std::shared_ptr<JuceImpl> impl;
};
