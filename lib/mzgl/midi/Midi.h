//
//  MZOpenGLView.h
//  MZGL
//
//  Created by Marek Bereza on 15/01/2018.
//  Copyright © 2018 Marek Bereza. All rights reserved.
//
#pragma once

#include <functional>
#include <string>
//#include <algorithm>
#include "stringUtil.h"
#include "mzgl/util/log.h"
#include <memory>

#include "RtMidi.h"

#include "log.h"

void MidiInCallback(double deltatime, std::vector<unsigned char> *message, void *userData);
#include "MidiMessage.h"

/**
 * This is a midi device info object, hopefully will have more in it.
 */
class MidiDevice {
public:
	std::string name;
};

class MidiListener {
public:
	virtual void midiReceived(const MidiDevice &device, const MidiMessage &m, uint64_t timestamp) = 0;
};

class MidiPort : public MidiDevice {
public:
	virtual RtMidi *getPort() = 0;
	virtual bool isOutput()	  = 0;

	virtual ~MidiPort() {}

	bool isOpen() { return getPort()->isPortOpen(); }

	void close() { getPort()->closePort(); }

	std::string getName() const { return name; }
	std::vector<std::string> getPortNames() { return MidiPort::getPortNames(isOutput(), false); }

	static int findPortByName(std::string portSearchName, bool isOutput) {
		auto ports						   = MidiPort::getPortNames(isOutput, false);
		std::string portSearchNameOriginal = portSearchName;
		portSearchName					   = toLowerCase(portSearchName);

		for (int i = 0; i < ports.size(); i++) {
			ports[i] = toLowerCase(ports[i]);
			if (ports[i].find(portSearchName) != -1) {
				Log::v() << "Connecting to port '" << portSearchNameOriginal << "' ('" + ports[i] + "')";
				return i;
			}
		}
		Log::e() << "Error can't find midi port " << portSearchNameOriginal;
		return -1;
	}

	static std::vector<std::string> getPortNames(bool isOutput, bool alsoPrint = false) {
		std::vector<std::string> names;

		RtMidiIn midiIn;
		RtMidiOut midiOut;
		RtMidi *midi;

		if (isOutput) {
			midi = &midiOut;
		} else {
			midi = &midiIn;
		}

		try {
			std::string io	= isOutput ? "output" : "input";
			uint32_t nPorts = midi->getPortCount();
			if (alsoPrint) {
				std::cout << "\nThere are " << nPorts << " MIDI " << io << " ports available.\n";
			}

			std::string portName;
			for (uint32_t i = 0; i < nPorts; i++) {
				try {
					portName = midi->getPortName(i);
					names.push_back(portName);
				} catch (RtMidiError &error) {
					error.printMessage();
				}
				if (alsoPrint) std::cout << "  " << io << " port #" << i << ": " << portName << '\n';
			}
		} catch (...) {
			Log::e() << "RtMidi error";
		}

		return names;
	}

	bool open(std::string portSearchName) {
		int portId = MidiPort::findPortByName(portSearchName, isOutput());
		if (portId != -1) {
			return open(portId);
		}
		return false;
	}

	bool open(int port = 0) {
		if (isOpen()) close();
		getPort()->openPort(port);
		return true;
	}
};

class MidiIn : public MidiPort {
public:
	static std::vector<std::string> getPortNames(bool alsoPrint = false) {
		return MidiPort::getPortNames(false, alsoPrint);
	}

	MidiIn() {
		try {
			rtMidiIn = std::shared_ptr<RtMidiIn>(new RtMidiIn());

			bool ignoreMidiSysex = true;
			bool ignoreMidiTime	 = false;
			bool ignoreMidiSense = true;

			rtMidiIn->ignoreTypes(ignoreMidiSysex, ignoreMidiTime, ignoreMidiSense);

			rtMidiIn->setCallback(&MidiInCallback, (void *) this);
		} catch (RtMidiError &error) {
			error.printMessage();
		}
	}

	virtual ~MidiIn() {
		if (rtMidiIn) rtMidiIn->closePort();
	}

	RtMidi *getPort() override { return rtMidiIn.get(); }
	bool isOutput() override { return false; }

	void callback(double deltatime, std::vector<unsigned char> *message) {
		MidiMessage m(*message);
		for (auto *l: listeners)
			l->midiReceived(*this, m, deltatime);
	}

	void addListener(MidiListener *listener) { listeners.push_back(listener); }

	void removeListener(MidiListener *listener) {
		for (int i = 0; i < listeners.size(); i++) {
			if (listeners[i] == listener) listeners.erase(listeners.begin() + i);
			return;
		}
	}

	void removeAllListeners() { listeners.clear(); }

	std::vector<MidiListener *> listeners;

	static int getNumPorts() {
		try {
			RtMidiIn midiIn;
			return midiIn.getPortCount();
		} catch (RtMidiError &error) {
			error.printMessage();
		} catch (...) {
			printf("Definitely something went wrong\n");
		}
		return 0;
	}

private:
	std::shared_ptr<RtMidiIn> rtMidiIn;
};

class MidiOut : public MidiPort {
public:
	bool isOutput() override { return true; }
	RtMidi *getPort() override { return rtMidiOut.get(); }

	static std::vector<std::string> getPortNames(bool alsoPrint = false) {
		return MidiPort::getPortNames(true, alsoPrint);
	}

	MidiOut() {
		try {
			rtMidiOut = std::make_shared<RtMidiOut>();
		} catch (RtMidiError &error) {
			// Handle the exception here
			error.printMessage();
		}
	}

	virtual ~MidiOut() {
		//logger::v() << "Deleting midiout";
		if (rtMidiOut) rtMidiOut->closePort();
	}

	void noteOn(int channel, int pitch, int velocity) {
		MidiMessage m;
		m.noteOn(channel, pitch, velocity);
		sendMessage(m);
	}

	void noteOff(int channel, int pitch) {
		MidiMessage m;
		m.noteOff(channel, pitch);
		sendMessage(m);
	}

	void cc(int channel, int control, int value) {
		MidiMessage m;
		m.cc(channel, control, value);
		sendMessage(m);
	}

	void sendMessage(const MidiMessage &m) {
		auto b = m.getBytes();
		rtMidiOut->sendMessage(&b);
	}

	void sendMessage(const std::vector<uint8_t> &msg) { rtMidiOut->sendMessage(&msg); }

private:
	std::shared_ptr<RtMidiOut> rtMidiOut;
};
