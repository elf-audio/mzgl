//
// Created by Marek Bereza on 26/02/2024.
//

#pragma once

class AllMidiDevicesRtMidiImpl
	: public MidiListener
	, public AllMidiDevicesImpl {
public:
	void setup() override { autoPoll(); }

	// sends to all
	void sendMessage(const MidiMessage &m) override { Log::e() << "Unimplemented"; }

	// send message to a specific device
	void sendMessage(const std::shared_ptr<MidiDevice> &device, const MidiMessage &m) override {
		Log::e() << "Unimplemented";
	}

	std::vector<std::shared_ptr<MidiDevice>> getConnectedMidiDevices() const override {
		std::vector<std::shared_ptr<MidiDevice>> devices;
		for (auto &m: midiIns) {
			devices.push_back(m.second);
		}
		for (auto &m: midiOuts) {
			devices.push_back(m.second);
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

	void midiReceived(const std::shared_ptr<MidiDevice> &device,
					  const MidiMessage &m,
					  uint64_t timestamp) override {
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