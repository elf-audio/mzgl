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
#include "MidiMessage.h"
#include "Midi.h"
#include "AllMidiDevicesImpl.h"
class CoreMidiDevice : public MidiDevice {
public:
	MIDIEndpointRef endpoint;

	bool isNetworkSession = false;
	// this populates the name field
	CoreMidiDevice(MIDIEndpointRef endpoint);
};

class CoreMidiSource : public CoreMidiDevice {
public:
	static std::shared_ptr<CoreMidiSource> create(MIDIEndpointRef endpoint) {
		return std::shared_ptr<CoreMidiSource>(new CoreMidiSource(endpoint));
	}

private:
	CoreMidiSource(MIDIEndpointRef endpoint)
		: CoreMidiDevice(endpoint) {
		this->name = name;
	}
};

class CoreMidiDestination : public CoreMidiDevice {
public:
	static std::shared_ptr<CoreMidiDestination> create(MIDIEndpointRef endpoint) {
		return std::shared_ptr<CoreMidiDestination>(new CoreMidiDestination(endpoint));
	}

private:
	CoreMidiDestination(MIDIEndpointRef endpoint)
		: CoreMidiDevice(endpoint) {}
};

typedef std::shared_ptr<CoreMidiDevice> CoreMidiDeviceRef;
typedef std::shared_ptr<CoreMidiSource> CoreMidiSourceRef;
typedef std::shared_ptr<CoreMidiDestination> CoreMidiDestinationRef;

class AllMidiDevicesAppleImpl : public AllMidiDevicesImpl {
public:
	MIDIClientRef client					   = 0;
	MIDIPortRef inputPort					   = 0;
	MIDIPortRef outputPort					   = 0;
	MIDIEndpointRef virtualDestinationEndpoint = 0;
	MIDIEndpointRef virtualSourceEndpoint	   = 0;

	std::vector<CoreMidiSourceRef> sources;
	std::vector<CoreMidiDestinationRef> destinations;
	std::vector<MidiListener *> listeners;

	void setup() override;

	void addListener(MidiListener *l) override { listeners.push_back(l); }

	void scanForDevices();

	void midiReceived(const MidiDevice &device, const MidiMessage &msg, uint64_t timestamp);

	void sendMessage(const MidiMessage &m) override;

	virtual ~AllMidiDevicesAppleImpl();

	// needs to be public because it's called by callbacks
	void packetListReceived(const CoreMidiDevice &device, const MIDIPacketList *packetList);
	void midiNotify(const MIDINotification *notification);

private:
	std::atomic<bool> running {false};
	std::thread portScannerThread;
	void autoPoll();

	void connectDestination(MIDIEndpointRef endpoint);
	void disconnectDestination(MIDIEndpointRef endpoint);

	void connectSource(MIDIEndpointRef endpoint);
	void disconnectSource(MIDIEndpointRef endpoint);
	CoreMidiSourceRef getSource(MIDIEndpointRef endpoint);
	CoreMidiDestinationRef getDestination(MIDIEndpointRef endpoint);

	std::vector<unsigned char> pendingMsg;

	void midiNotifyAdd(const MIDIObjectAddRemoveNotification *notification);
	void midiNotifyRemove(const MIDIObjectAddRemoveNotification *notification);

	void sendPacketList(const MIDIPacketList *packetList);
	void sendBytes(const UInt8 *data, UInt32 size);
	void sendSysex(const UInt8 *data, UInt32 size);
};
