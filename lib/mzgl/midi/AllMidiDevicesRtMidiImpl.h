//
// Created by Marek Bereza on 26/02/2024.
//

#pragma once

#include "RtMidiPorts.h"

#include <atomic>
#include <memory>

// A shared_ptr several threads swap and read. std::atomic<std::shared_ptr> where the library has it,
// otherwise the C++11 free functions it replaced (deprecated in C++20).
template <typename T>
class AtomicSharedPtr {
public:
	explicit AtomicSharedPtr(std::shared_ptr<T> initial)
		: ptr(std::move(initial)) {}

#ifdef __cpp_lib_atomic_shared_ptr
	std::shared_ptr<T> load() const {
		return ptr.load();
	}
	void store(std::shared_ptr<T> value) {
		ptr.store(std::move(value));
	}

private:
	std::atomic<std::shared_ptr<T>> ptr;
#else
	std::shared_ptr<T> load() const {
		return std::atomic_load(&ptr);
	}
	void store(std::shared_ptr<T> value) {
		std::atomic_store(&ptr, std::move(value));
	}

private:
	std::shared_ptr<T> ptr;
#endif
};

class AllMidiDevicesRtMidiImpl
	: public MidiListener
	, public AllMidiDevicesImpl {
public:
	void setup() override { autoPoll(); }

	// sends to all
	void sendMessage(const MidiMessage &m) override {
		const auto outputs = outputSnapshot.load();
		for (auto &output: *outputs) {
			send(*output, m);
		}
	}

	// send message to a specific device. RtMidi can't schedule, so the delay is ignored and the
	// message goes out now
	void sendMessage(const std::shared_ptr<MidiDevice> &device,
					 const MidiMessage &m,
					 std::optional<uint64_t> delayInNanoSeconds) override {
		const auto outputs = outputSnapshot.load();
		for (auto &output: *outputs) {
			if (*output == *device) {
				send(*output, m);
				return;
			}
		}
	}

	void sendMessageToMatching(const MidiMessage &m,
							   std::optional<uint64_t> delayInNanoSeconds,
							   const std::function<bool(const MidiDevice &)> &shouldSendTo) override {
		const auto outputs = outputSnapshot.load();
		for (auto &output: *outputs) {
			if (shouldSendTo(*output)) {
				send(*output, m);
			}
		}
	}

	std::vector<std::shared_ptr<MidiDevice>> getConnectedMidiDevices() const override {
		const auto inputs  = inputSnapshot.load();
		const auto outputs = outputSnapshot.load();
		std::vector<std::shared_ptr<MidiDevice>> devices(std::begin(*inputs), std::end(*inputs));
		devices.insert(std::end(devices), std::begin(*outputs), std::end(*outputs));
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

	void midiReceived(const std::shared_ptr<MidiDevice> &device,
					  const MidiMessage &m,
					  uint64_t timestamp) override {
		for (auto l: listeners) {
			l->midiReceived(device, m, timestamp);
		}
	}

	~AllMidiDevicesRtMidiImpl() {
		// Stop the scanner first - it adds/erases entries in midiIns from its
		// own thread, so closing the ports while it runs is a race.
		if (running) {
			running = false;
			portScannerThread.join();
		}
		// MidiPort::close() swallows RtMidiError: this is a destructor, so a
		// throw here (e.g. midiInUnprepareHeader failing on Windows) would be
		// std::terminate.
		for (auto &m: midiIns) {
			m.second->close();
		}
	}

private:
	using Inputs  = const std::vector<std::shared_ptr<MidiIn>>;
	using Outputs = const std::vector<std::shared_ptr<MidiOut>>;

	// only touched by the scanner thread (and the destructor once it has stopped)
	std::map<std::string, std::shared_ptr<MidiIn>> midiIns;
	std::map<std::string, std::shared_ptr<MidiOut>> midiOuts;

	// Outputs dropped by the last poll. A sender (the audio thread, for MIDI clock out) may still
	// hold the previous snapshot, and if it let go of the last reference it would close the port
	// itself, so keep them for one more poll and close them here.
	std::vector<std::shared_ptr<MidiOut>> retiredOutputs;

	// What everyone else sees: swapped whole by the scanner
	AtomicSharedPtr<Inputs> inputSnapshot {std::make_shared<Inputs>()};
	AtomicSharedPtr<Outputs> outputSnapshot {std::make_shared<Outputs>()};

	std::atomic<bool> running {false};
	std::thread portScannerThread;

	static void send(MidiOut &output, const MidiMessage &m) {
		// RtMidi reports driver errors by throwing - never let that out (this runs on the audio thread)
		try {
			output.sendMessage(m);
		} catch (const std::exception &e) {
			Log::e() << "Failed to send MIDI to '" << output.name << "': " << e.what();
		}
	}

	// On ALSA our own open ports show up in the port lists like anyone else's (an RtMidiIn's port
	// is writable, so it's listed as an output). Talking to them would feed our output straight
	// back into our input.
	static bool isOurOwnPort(const std::string &name) {
#ifdef __linux__
		return startsWith(name, "RtMidi Input Client:", CaseSensitivity::caseSensitive)
			   || startsWith(name, "RtMidi Output Client:", CaseSensitivity::caseSensitive);
#else
		return false;
#endif
	}

	template <typename Port>
	static void open(Port &port, int index, const std::string &name) {
		// Opening a port can throw (e.g. on Windows MM an input is exclusive and may already be held
		// by another process/instance). Don't let that take down the whole app - just skip this port
		// and carry on.
		try {
			port.open(index);
		} catch (const std::exception &e) {
			Log::e() << "Failed to open MIDI port '" << name << "': " << e.what();
		} catch (...) {
			Log::e() << "Failed to open MIDI port '" << name << "'";
		}
	}

	// Brings ports in line with the system's current list. Returns whether anything changed.
	template <typename Port>
	bool update(std::map<std::string, std::shared_ptr<Port>> &ports,
				const std::vector<std::string> &names,
				const std::function<void(Port &, int, const std::string &)> &openPort,
				std::vector<std::shared_ptr<Port>> &removed) {
		bool changed = false;
		for (int i = 0; i < names.size(); i++) {
			if (isOurOwnPort(names[i]) || ports.find(names[i]) != ports.end()) continue;
			// new midi port! Record it first so we don't rescan/retry it every poll.
			auto port  = std::make_shared<Port>();
			port->name = names[i];
			Log::d() << "Added port " << names[i];
			ports[names[i]] = port;
			openPort(*port, i, names[i]);
			changed = true;
		}

		const std::set<std::string> nameSet(std::begin(names), std::end(names));
		for (auto it = std::begin(ports); it != std::end(ports);) {
			if (nameSet.find(it->first) == nameSet.end()) {
				Log::d() << "Removing port " << it->first;
				removed.push_back(it->second);
				it		= ports.erase(it);
				changed = true;
			} else {
				++it;
			}
		}
		return changed;
	}

	void poll() {
		retiredOutputs.clear();

		std::vector<std::shared_ptr<MidiIn>> removedIns;
		const bool insChanged = update<MidiIn>(
			midiIns,
			MidiIn::getPortNames(),
			[this](MidiIn &in, int index, const std::string &name) {
				open(in, index, name);
				in.addListener(this);
			},
			removedIns);
		if (insChanged) {
			auto inputs = std::make_shared<std::vector<std::shared_ptr<MidiIn>>>();
			for (auto &in: midiIns) {
				inputs->push_back(in.second);
			}
			inputSnapshot.store(std::move(inputs));
		}

		const bool outsChanged = update<MidiOut>(
			midiOuts,
			MidiOut::getPortNames(),
			[](MidiOut &out, int index, const std::string &name) { open(out, index, name); },
			retiredOutputs);
		if (outsChanged) {
			// only list outputs we could open, so nobody sends to a closed port
			auto outputs = std::make_shared<std::vector<std::shared_ptr<MidiOut>>>();
			for (auto &out: midiOuts) {
				if (out.second->isOpen()) outputs->push_back(out.second);
			}
			outputSnapshot.store(std::move(outputs));
		}
	}
};
