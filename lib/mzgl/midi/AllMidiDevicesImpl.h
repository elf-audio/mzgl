//
//  AllMidiDevicesImpl.h
//  mzgl
//
//  Created by Marek Bereza on 13/11/2021.
//  Copyright © 2021 Marek Bereza. All rights reserved.
//

#pragma once

class MidiListener;
class MidiMessage;

class AllMidiDevicesImpl {
public:
	virtual void setup() {}
	virtual void addListener(MidiListener *l) = 0;

	// send message to all connected midi devices that have an input
	virtual void sendMessage(const MidiMessage &m) = 0;
};
