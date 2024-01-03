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
	void notifyConnectionChange() {
		for (auto *l: connectionListeners) {
			l->midiConnectionsChanged();
		}
	}
	virtual void setup() {}
	virtual void addListener(MidiListener *l) = 0;

	// send message to all connected midi devices that have an input
	virtual void sendMessage(const MidiMessage &m) = 0;
};
