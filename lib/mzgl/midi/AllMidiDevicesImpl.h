//
//  AllMidiDevicesImpl.h
//  mzgl
//
//  Created by Marek Bereza on 13/11/2021.
//  Copyright © 2021 Marek Bereza. All rights reserved.
//

#pragma once
#include "AllMidiDevices.h"

class AllMidiDevicesImpl {
public:
	std::vector<MidiConnectionListener *> connectionListeners;

	void addConnectionListener(MidiConnectionListener *l) { connectionListeners.push_back(l); }
	void removeConnectionListener(MidiConnectionListener *l) {
		connectionListeners.erase(std::remove(connectionListeners.begin(), connectionListeners.end(), l),
								  connectionListeners.end());
	}
	void notifyConnection(const MidiDevice &dev) {
		for (auto *l: connectionListeners) {
			l->midiDeviceConnected(dev);
		}
	}

	void notifyDisconnection(const MidiDevice &dev) {
		for (auto *l: connectionListeners) {
			l->midiDeviceDisconnected(dev);
		}
	}
	virtual ~AllMidiDevicesImpl() {}
	virtual std::vector<MidiDevice> getConnectedMidiDevices() = 0;
	virtual void setup() {}

	virtual void addListener(MidiListener *l) = 0;

	// send message to all connected midi devices that have an input
	virtual void sendMessage(const MidiMessage &m) = 0;
};
