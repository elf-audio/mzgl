//
//  AllMidiDevicesLibremidiImpl.h
//  mzgl
//
//  Windows/Linux midi, on libremidi (WinMM / ALSA sequencer). Kept free of libremidi headers so they
//  don't leak into every translation unit.
//
#pragma once

#include "AllMidiDevicesImpl.h"

#include <atomic>
#include <condition_variable>
#include <map>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

class MainThreadRunner;

namespace mzgl::midi {
	class Input;
	class Output;
	class PortWatcher;

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
} // namespace mzgl::midi

class AllMidiDevicesLibremidiImpl
	: public MidiListener
	, public AllMidiDevicesImpl
	, public std::enable_shared_from_this<AllMidiDevicesLibremidiImpl> {
public:
	AllMidiDevicesLibremidiImpl();
	~AllMidiDevicesLibremidiImpl() override;

	void setup() override;

	// Connection notifications are delivered on this runner's thread, and only while there is one:
	// changes made in the meantime are announced when it's set again.
	void setMainThreadRunner(MainThreadRunner *runner);

	// sends to all
	void sendMessage(const MidiMessage &m) override;

	// send message to a specific device. Neither backend schedules, so the delay is ignored and the
	// message goes out now
	void sendMessage(const std::shared_ptr<MidiDevice> &device,
					 const MidiMessage &m,
					 std::optional<uint64_t> delayInNanoSeconds) override;

	void sendMessageToMatching(const MidiMessage &m,
							   std::optional<uint64_t> delayInNanoSeconds,
							   const std::function<bool(const MidiDevice &)> &shouldSendTo) override;

	// Only what has been announced, so this agrees with the connection notifications listeners have had
	std::vector<std::shared_ptr<MidiDevice>> getConnectedMidiDevices() const override;

	void
		midiReceived(const std::shared_ptr<MidiDevice> &device, const MidiMessage &m, uint64_t timestamp) override;

private:
	using Inputs	= const std::vector<std::shared_ptr<mzgl::midi::Input>>;
	using Outputs	= const std::vector<std::shared_ptr<mzgl::midi::Output>>;
	using Announced = const std::vector<std::shared_ptr<MidiDevice>>;

	// Everything below up to the snapshots is only touched by the worker thread (and the destructor
	// once it has stopped): ports are opened, closed and listed there and nowhere else.
	std::unique_ptr<mzgl::midi::PortWatcher> watcher;
	std::map<std::string, std::shared_ptr<mzgl::midi::Input>> inputs;
	std::map<std::string, std::shared_ptr<mzgl::midi::Output>> outputs;

	// Outputs dropped by a rescan. A sender (the audio thread, for MIDI clock out) may still hold the
	// previous snapshot, and if it let go of the last reference it would close the port itself, so
	// keep them until nobody else does and close them on the worker thread.
	std::vector<std::shared_ptr<mzgl::midi::Output>> retiredOutputs;

	// What everyone else sees: swapped whole by the worker thread
	mzgl::midi::AtomicSharedPtr<Inputs> inputSnapshot {std::make_shared<Inputs>()};
	mzgl::midi::AtomicSharedPtr<Outputs> outputSnapshot {std::make_shared<Outputs>()};

	// What has been announced to the connection listeners. Changed on the main thread only.
	mzgl::midi::AtomicSharedPtr<Announced> announcedSnapshot {std::make_shared<Announced>()};

	std::mutex runnerMutex;
	MainThreadRunner *mainThreadRunner = nullptr;

	// The worker sleeps until the system says the ports changed (or the fallback timeout), then rescans
	std::thread worker;
	std::mutex workerMutex;
	std::condition_variable workerWake;
	bool rescanRequested = true;
	bool stopRequested	 = false;

	void requestRescan();
	void run();
	void rescan();

	void requestAnnouncement();
	void announceChanges();
};
