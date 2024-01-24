//
//  AllMidiDevicesApple.h
//  mzgl
//
//  Created by Marek Bereza on 13/11/2021.
//  Copyright © 2021 Marek Bereza. All rights reserved.
//

#pragma once
#include <vector>
#include <string>
#include <thread>

#include <CoreMIDI/CoreMIDI.h>
#include <mzgl/midi/MidiMessage.h>
#include <mzgl/midi/Midi.h>
#include "AllMidiDevicesImpl.h"
class CoreMidiDevice : public MidiDevice {
public:
	MIDIEndpointRef endpoint;
	explicit CoreMidiDevice(MIDIEndpointRef endpoint);
	~CoreMidiDevice() override = default;
};

class CoreMidiIn : public CoreMidiDevice {
public:
	static std::shared_ptr<CoreMidiIn> create(MIDIEndpointRef endpoint) {
		return std::shared_ptr<CoreMidiIn>(new CoreMidiIn(endpoint));
	}
	~CoreMidiIn() override = default;

private:
	explicit CoreMidiIn(MIDIEndpointRef endpoint)
		: CoreMidiDevice(endpoint) {
		isOutput = false;
	}
};

class CoreMidiOut : public CoreMidiDevice {
public:
	static std::shared_ptr<CoreMidiOut> create(MIDIEndpointRef endpoint) {
		return std::shared_ptr<CoreMidiOut>(new CoreMidiOut(endpoint));
	}
	~CoreMidiOut() override = default;

private:
	explicit CoreMidiOut(MIDIEndpointRef endpoint)
		: CoreMidiDevice(endpoint) {
		isOutput = true;
	}
};

typedef std::shared_ptr<CoreMidiDevice> CoreMidiDeviceRef;
typedef std::shared_ptr<CoreMidiIn> CoreMidiInRef;
typedef std::shared_ptr<CoreMidiOut> CoreMidiOutRef;

class AllMidiDevicesAppleImpl : public AllMidiDevicesImpl {
public:
	MIDIClientRef client					   = 0;
	MIDIPortRef inputPort					   = 0;
	MIDIPortRef outputPort					   = 0;
	MIDIEndpointRef virtualDestinationEndpoint = 0;
	MIDIEndpointRef virtualSourceEndpoint	   = 0;

	void setup() override;

	void scanForDevices();

	void midiReceived(const std::shared_ptr<MidiDevice> &device, const MidiMessage &msg, uint64_t timestamp);

	void sendMessage(const MidiMessage &m) override;
	void sendMessage(const std::shared_ptr<MidiDevice> &device, const MidiMessage &m) override;
	~AllMidiDevicesAppleImpl() override;

	std::vector<std::shared_ptr<MidiDevice>> getConnectedMidiDevices() const override;

	// needs to be public because it's called by callbacks
	void packetListReceived(const CoreMidiDevice &device, const MIDIPacketList *packetList);
	void midiNotify(const MIDINotification *notification);

private:
	std::vector<CoreMidiInRef> midiIns;
	std::vector<CoreMidiOutRef> midiOuts;

	std::atomic<bool> running {false};
	std::thread portScannerThread;
	void autoPoll();

	void connectOutput(MIDIEndpointRef endpoint);
	void disconnectOutput(MIDIEndpointRef endpoint);

	void connectInput(MIDIEndpointRef endpoint);
	void disconnectInput(MIDIEndpointRef endpoint);
	CoreMidiInRef getInput(MIDIEndpointRef endpoint);
	CoreMidiOutRef getOutput(MIDIEndpointRef endpoint);

	std::vector<unsigned char> pendingMsg;

	void midiNotifyAdd(const MIDIObjectAddRemoveNotification *notification);
	void midiNotifyRemove(const MIDIObjectAddRemoveNotification *notification);

	void sendBytes(const std::shared_ptr<MidiDevice> &device, const MidiMessage &midiMessage);
	void sendSysex(const std::shared_ptr<MidiDevice> &device, const MidiMessage &midiMessage);
};
