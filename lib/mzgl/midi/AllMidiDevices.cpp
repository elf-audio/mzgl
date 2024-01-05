//
//  AllMidiDevices.cpp
//  mzgl
//
//  Created by Marek Bereza on 13/11/2021.
//  Copyright © 2021 Marek Bereza. All rights reserved.
//

#include "AllMidiDevices.h"
#include "AllMidiDevicesImpl.h"
#include <atomic>
#include "mainThread.h"
#include "util.h"

#ifdef __APPLE__
#	include "AllMidiDevicesAppleImpl.h"
#endif

#ifdef __ANDROID__
class AllMidiDevicesAndroidImpl : public AllMidiDevicesImpl {
	void setup() override { androidSetupAllMidiIns(); }

	void addListener(MidiListener *listener) override { androidAddMidiListener(listener); }
    void removeListener(MidiListener *listener) override { androidRemoveMidiListener(listener); }

	void sendMessage(const MidiMessage &m) override { Log::e() << "AllMidiDevicesAndroidImpl::sendMessage() Unimplemented"; }
    void sendMessage(const MidiDevice &device, const MidiMessage &m) override { Log::e() << "AllMidiDevicesAndroidImpl::sendMessage() Unimplemented";}


    std::vector<MidiDevice> getConnectedMidiDevices() override {
        Log::e() << "AllMidiDevicesAndroidImpl::getConnectedMidiDevices() unimplemented";
        return {};
    }
};
#else

class AllMidiDevicesRtMidiImpl
	: public MidiListener
	, public AllMidiDevicesImpl {
public:
	void setup() override { autoPoll(); }

	void addListener(MidiListener *listener) override { listeners.push_back(listener); }
	void removeListener(MidiListener *listener) override {
		listeners.erase(std::remove(listeners.begin(), listeners.end(), listener), listeners.end());
	}
	// sends to all
	void sendMessage(const MidiMessage &m) override { Log::e() << "Unimplemented"; }

	std::vector<MidiDevice> getConnectedMidiDevices() override {
		std::vector<MidiDevice> devices;
		for (auto &m: midiIns) {
			devices.push_back(*m.second);
		}
		for (auto &m: midiOuts) {
			devices.push_back(*m.second);
		}
		return devices;
	}

	void autoPoll() {
		if (running) return;
		// keep checking for new ports
		running			  = true;
		portScannerThread = std::thread([this]() {
			while (running) {
				//                main.runOnMainThread([this] {
				// this could crash on some OS's if you need to call this on main thread
				poll();
				//                });
				for (int i = 0; i < 100; i++) {
					sleepMillis(10);
					if (!running) break;
				}
			}
		});
	}

	std::set<std::string> setFromVector(std::vector<std::string> &names) {
		std::set<std::string> nameSet;
		for (auto &name: names)
			nameSet.insert(std::move(name));
		names.clear();
		return nameSet;
	}

	void midiReceived(const MidiDevice &device, const MidiMessage &m, uint64_t timestamp) override {
		for (auto l: listeners) {
			l->midiReceived(device, m, timestamp);
		}
	}

	~AllMidiDevicesRtMidiImpl() {
		for (auto m: midiIns) {
			m.second->close();
		}
		if (running) {
			running = false;
			portScannerThread.join();
		}
	}

private:
	std::map<std::string, std::shared_ptr<MidiIn>> midiIns;
	std::map<std::string, std::shared_ptr<MidiOut>> midiOuts;
	std::atomic<bool> running {false};
	std::thread portScannerThread;
	std::vector<MidiListener *> listeners;

	// must be called on main thread
	void poll() {
		int newNumPorts = MidiIn::getNumPorts();

		if (newNumPorts != midiIns.size()) {
			auto names = MidiIn::getPortNames();
			for (int i = 0; i < names.size(); i++) {
				if (midiIns.find(names[i]) == midiIns.end()) {
					// new midi port!
					auto m = std::make_shared<MidiIn>();
					Log::d() << "Added port " << names[i] << " must properly debug this with lots of devices!!";
					midiIns[names[i]] = m;
					m->open(i);
					m->addListener(this);
				}
			}

			std::set<std::string> nameSet = setFromVector(names);

			std::vector<std::string> namesToErase;

			for (auto &midiIn: midiIns) {
				const std::string &name = midiIn.first;
				if (nameSet.find(name) == nameSet.end()) {
					namesToErase.push_back(name);
				}
			}

			for (auto &n: namesToErase) {
				midiIns.erase(n);
				Log::d() << "Removing port " << n << " must properly debug this with lots of devices!!";
			}
		}
	}
};

#endif

AllMidiDevices::AllMidiDevices(bool online)
	: online(online) {
	if (online) {
#ifdef FORCE_RTMIDI
		impl = std::make_shared<AllMidiDevicesRtMidiImpl>();
#elif defined(__ANDROID__)
		impl = std::make_shared<AllMidiDevicesAndroidImpl>();
#elif defined(__APPLE__)
		impl = std::make_shared<AllMidiDevicesAppleImpl>();
#else
		impl = std::make_shared<AllMidiDevicesRtMidiImpl>();
#endif
	}
}

void AllMidiDevices::setup() {
	if (online) {
		impl->setup();
	}
}
void AllMidiDevices::addListener(MidiListener *listener) {
	if (online) {
		impl->addListener(listener);
	}
}

void AllMidiDevices::sendMessage(const MidiMessage &m) {
	if (online) {
		impl->sendMessage(m);
	}
}
void AllMidiDevices::addConnectionListener(MidiConnectionListener *listener) {
	if (online) {
		impl->addConnectionListener(listener);
	}
}

void AllMidiDevices::removeConnectionListener(MidiConnectionListener *listener) {
	if (online) {
		impl->removeConnectionListener(listener);
	}
}

std::vector<MidiDevice> AllMidiDevices::getConnectedMidiDevices() {
	if (online) {
		return impl->getConnectedMidiDevices();
	} else {
		return {};
	}
}

void AllMidiDevices::sendMessage(const MidiDevice &device, const MidiMessage &m) {
	if (online) {
		impl->sendMessage(device, m);
	}
}
void AllMidiDevices::removeListener(MidiListener *listener) {
	if (online) {
		impl->removeListener(listener);
	}
}