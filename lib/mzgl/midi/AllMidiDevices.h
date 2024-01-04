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
#	import "androidUtil.h"
#endif

class MidiConnectionListener {
public:
	virtual void midiDeviceConnected(const MidiDevice &dev) {}
	virtual void midiDeviceDisconnected(const MidiDevice &dev) {}
};

class AllMidiDevicesImpl;

// this forces the RtMidi implementation for testing on mac
// not to be used in production.
//#define FORCE_RTMIDI

class AllMidiDevices {
public:
	// you can disable actual midi for testing or offline rendering by passing in online = false
	AllMidiDevices(bool online = true);

	void setup();

	void addListener(MidiListener *listener);
	void removeListener(MidiListener *listener);

	void addConnectionListener(MidiConnectionListener *listener);
	void removeConnectionListener(MidiConnectionListener *listener);

	// sends to all connected midi devices that have an input
	void sendMessage(const MidiMessage &m);
	void sendMessage(const MidiDevice &device, const MidiMessage &m);

	std::vector<MidiDevice> getConnectedMidiDevices();

private:
	const bool online;
	std::shared_ptr<AllMidiDevicesImpl> impl;
};
