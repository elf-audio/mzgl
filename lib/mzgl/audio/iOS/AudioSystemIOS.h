//
//  Header.h
//  MZGLiOS
//
//  Created by Marek Bereza on 23/01/2018.
//  Copyright © 2018 Marek Bereza. All rights reserved.
//

#pragma once
#include "AudioSystem.h"
#include <AudioToolbox/AudioToolbox.h>

class AudioSystemIOS : public _AudioSystem {
public:
	AudioSystemIOS();
	~AudioSystemIOS() override;
	void setup(int numInputs = 2, int numOutputs = 2) override;
	void start() override;
	void stop() override;

	bool isRunning() override { return running; }
	[[nodiscard]] bool isInsideAudioCallback() override { return isRunning(); }

	std::vector<AudioPort> getInputs() override;
	std::vector<AudioPort> getOutputs() override;

	AudioUnit audioUnit;
	void handleRouteChange();

private:
	std::atomic<bool> running {false};

	// iOS specific, should move to specific implementaiton
	void configureAudioSession();
	void configureAudioUnit();
	void deleteContext(void *ctx);
	std::string auErrString(OSStatus status);
	bool checkStatus(OSStatus status);
};

typedef AudioSystemIOS AudioSystem;
