//
//  AudioSystemRtAudio.hpp
//  MZGL
//
//  Created by Marek Bereza on 23/01/2018.
//  Copyright © 2018 Marek Bereza. All rights reserved.
//

#pragma once
#include "AudioSystem.h"

class RtAudio;
class AudioSystemRtAudio : public _AudioSystem {
public:
	AudioSystemRtAudio();
	~AudioSystemRtAudio() override;
	void setup(int numInChannels, int numOutChannels) override;
	void start() override;
	void stop() override;
	bool isRunning() override;
	
	
	std::vector<AudioPort> getInputs() override;
	std::vector<AudioPort> getOutputs() override;
	
private:
	bool running = false;
	std::shared_ptr<RtAudio> audio;

};

#ifdef __APPLE__
double getMacDefaultDeviceSampleRate();
#endif
