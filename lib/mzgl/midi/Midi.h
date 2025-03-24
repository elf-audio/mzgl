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
#include <atomic>
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
 * At the moment, it's copied around a lot, so to identify it use
 * the equality operator, which uses the id field which is a temporary
 * unique counter based id for the current app run.
 *
 * Also, don't store this by reference!
 */
class MidiDevice {
public:
	enum class Direction {
		Input,
		Output,
	};

	std::string name;
	int id;
	Direction direction = Direction::Input;

	MidiDevice(std::string name = "", Direction _direction = Direction::Input)
		: name(name)
		, direction(_direction) {
		static std::atomic<int> idCounter {0};
		id = idCounter++;
	}

	virtual ~MidiDevice() = default;

	[[nodiscard]] virtual bool operator==(const MidiDevice &other) const { return id == other.id; }
	[[nodiscard]] virtual bool operator!=(const MidiDevice &other) const { return id != other.id; }

private:
};

class MidiListener {
public:
	virtual void
		midiReceived(const std::shared_ptr<MidiDevice> &device, const MidiMessage &m, uint64_t timestamp) = 0;
};

class MidiPort : public MidiDevice {
public:
	virtual RtMidi *getPort() = 0;

	virtual ~MidiPort() {}

	bool isOpen() { return getPort()->isPortOpen(); }

	void close() { getPort()->closePort(); }

	std::string getName() const { return name; }
	std::vector<std::string> getPortNames() { return MidiPort::getPortNames(direction, false); }

	static int findPortByName(std::string portSearchName, MidiDevice::Direction direction) {
		auto ports						   = MidiPort::getPortNames(direction, false);
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

	static std::vector<std::string> getPortNames(MidiDevice::Direction direction, bool alsoPrint = false) {
		std::vector<std::string> names;

		RtMidiIn midiIn;
		RtMidiOut midiOut;
		RtMidi *midi;

		if (direction == MidiDevice::Direction::Output) {
			midi = &midiOut;
		} else {
			midi = &midiIn;
		}

		try {
			std::string io	= direction == MidiDevice::Direction::Output ? "output" : "input";
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
		int portId = MidiPort::findPortByName(portSearchName, direction);
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

class MidiIn
	: public MidiPort
	, public std::enable_shared_from_this<MidiIn> {
public:
	static std::vector<std::string> getPortNames(bool alsoPrint = false) {
		return MidiPort::getPortNames(MidiDevice::Direction::Input, alsoPrint);
	}

	MidiIn() {
		direction = MidiDevice::Direction::Input;
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

	void callback(double deltatime, std::vector<unsigned char> *message) {
		MidiMessage m(*message);
		for (auto *listener: listeners)
			listener->midiReceived(shared_from_this(), m, deltatime);
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
	RtMidi *getPort() override { return rtMidiOut.get(); }

	static std::vector<std::string> getPortNames(bool alsoPrint = false) {
		return MidiPort::getPortNames(MidiDevice::Direction::Output, alsoPrint);
	}

	MidiOut() {
		direction = MidiDevice::Direction::Output;
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
		sendMessage(MidiMessage::noteOn(channel, pitch, velocity));
	}

	void noteOff(int channel, int pitch) { sendMessage(MidiMessage::noteOff(channel, pitch)); }

	void cc(int channel, int control, int value) { sendMessage(MidiMessage::cc(channel, control, value)); }

	void sendMessage(const MidiMessage &m) {
		auto b = m.getBytes();
		rtMidiOut->sendMessage(&b);
	}

	void sendMessage(const std::vector<uint8_t> &msg) { rtMidiOut->sendMessage(&msg); }

private:
	std::shared_ptr<RtMidiOut> rtMidiOut;
};
