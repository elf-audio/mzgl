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

class MidiConnectionListener {
public:
	virtual ~MidiConnectionListener() = default;
	virtual void midiDeviceConnected(const std::shared_ptr<MidiDevice> &dev) {}
	virtual void midiDeviceDisconnected(const std::shared_ptr<MidiDevice> &dev) {}
};

class AllMidiDevicesImpl;
class MainThreadRunner;

class AllMidiDevices {
public:
	// you can disable actual midi for testing or offline rendering by passing in online = false
	AllMidiDevices(bool online = true);

	void setup();
	void setMainThreadRunner(MainThreadRunner *runner);

	void addListener(MidiListener *listener);
	void removeListener(MidiListener *listener);

	void addConnectionListener(MidiConnectionListener *listener);
	void removeConnectionListener(MidiConnectionListener *listener);

	// sends to all connected midi devices that have an input
	void sendMessage(const MidiMessage &m);
	void sendMessage(const std::shared_ptr<MidiDevice> &device,
					 const MidiMessage &m,
					 std::optional<uint64_t> delayInNanoSeconds);

	// sends to every connected device that shouldSendTo() accepts, without copying the device list
	// where the platform allows (Android). Safe to call from the audio thread there; keep the
	// predicate's captures to a couple of pointers so std::function doesn't allocate
	void sendMessageToMatching(const MidiMessage &m,
							   std::optional<uint64_t> delayInNanoSeconds,
							   const std::function<bool(const MidiDevice &)> &shouldSendTo);

	// builds a new list each call: for UI and setup, not per-message code
	std::vector<std::shared_ptr<MidiDevice>> getConnectedMidiDevices() const;

private:
	const bool online;
	std::shared_ptr<AllMidiDevicesImpl> impl;
};

class ScopedMidiConnectionListener {
public:
	ScopedMidiConnectionListener(AllMidiDevices &_devices, MidiConnectionListener *_listener)
		: devices(_devices)
		, listener(_listener) {
		devices.addConnectionListener(listener);
	}

	~ScopedMidiConnectionListener() { devices.removeConnectionListener(listener); }

private:
	AllMidiDevices &devices;
	MidiConnectionListener *listener;
};