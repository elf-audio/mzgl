//
//  AllMidiDevices.h
//  koala
//
//  Created by Marek Bereza on 20/11/2018.
//  Copyright © 2018 Marek Bereza. All rights reserved.
//

#pragma once
#include "Midi.h"
#include <thread>
#include <map>
#include <set>

#ifdef __ANDROID__
#import "androidUtil.h"
#endif


class AllMidiDevicesImpl;

class AllMidiDevices {
public:
    AllMidiDevices(bool online = true);
    std::shared_ptr<AllMidiDevicesImpl> impl;
    void setup();
    void addListener(MidiListener *listener);
	
	// sends to all connected midi devices that have an input
	void sendMessage(const MidiMessage &m);
private:
	const bool online;
	
};

	
