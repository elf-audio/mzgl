//
//  AllMidiDevicesLibremidiImpl.cpp
//  mzgl
//

#if defined(_WIN32)
// CM_Register_Notification (device change notifications) needs Windows 8 headers; the rest of mzgl
// targets 7
#	undef _WIN32_WINNT
#	define _WIN32_WINNT 0x0602
#endif

#include "AllMidiDevicesLibremidiImpl.h"

#include <libremidi/libremidi.hpp>

#include <algorithm>
#include <chrono>

#include "log.h"
#include "mainThread.h"
#include "stringUtil.h"

#if defined(_WIN32)
#	include <windows.h>
#	include <mmsystem.h>
#	include <cfgmgr32.h>
#endif

using namespace std::chrono_literals;

namespace mzgl::midi {

	// The name our own ports go by. On ALSA they're listed alongside everyone else's, so this is also
	// how we recognise them.
	static constexpr auto clientName = "Koala";

	static uint64_t nowNs() {
		// Listeners expect an absolute monotonic timestamp in nanoseconds (std::chrono::steady_clock, the
		// same clock the app reads for MIDI clock sync)
		const auto now = std::chrono::steady_clock::now().time_since_epoch();
		return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(now).count());
	}

	static bool failed(const stdx::error &err) {
		return err != stdx::error {};
	}

	static std::string describe(const stdx::error &err) {
		return std::string(err.message().data(), err.message().size());
	}

	static libremidi::input_api_configuration inputApi() {
#if defined(_WIN32)
		return libremidi::winmm_input_configuration {};
#else
		libremidi::alsa_seq::input_configuration conf;
		conf.client_name = clientName;
		return conf;
#endif
	}

#if !defined(_WIN32)
	static libremidi::output_api_configuration outputApi() {
		libremidi::alsa_seq::output_configuration conf;
		conf.client_name = clientName;
		return conf;
	}
#endif

	static bool isIgnored(const libremidi::port_information &port) {
#if defined(_WIN32)
		// The GS Wavetable Synth renders through its own waveOut on the default audio device, and holding
		// it open deadlocked winmm when that device changed (the port scan hung in midiInGetNumDevs and
		// all MIDI stopped), so leave it alone.
		return startsWith(port.port_name, "Microsoft GS Wavetable Synth", CaseSensitivity::caseSensitive);
#else
		// our own ports: talking to them would feed our output straight back into our input
		return port.device_name == clientName;
#endif
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////

	class Input
		: public MidiDevice
		, public std::enable_shared_from_this<Input> {
	public:
		Input(const std::string &name, MidiListener &listener)
			: MidiDevice(name, Direction::Input)
			, listener(listener) {}

		~Input() override { close(); }

		bool open(const libremidi::input_port &port) {
			libremidi::input_configuration conf;
			conf.on_message = [this](libremidi::message &&m) { received(m); };
			conf.on_error	= [this](std::string_view text, const auto &) {
				  Log::e() << "MIDI input '" << name << "': " << text;
			};
			conf.on_warning = [this](std::string_view text, const auto &) {
				Log::w() << "MIDI input '" << name << "': " << text;
			};
			// controllers like the SP-404MKII report button presses as sysex
			conf.ignore_sysex	= false;
			conf.ignore_timing	= false;
			conf.ignore_sensing = true;

			in = std::make_unique<libremidi::midi_in>(conf, inputApi());
			// On WinMM an input is exclusive, so this fails while another app has the port open
			if (auto err = in->open_port(port, clientName); failed(err)) {
				Log::e() << "Failed to open MIDI input '" << name << "': " << describe(err);
				in.reset();
				return false;
			}
			return true;
		}

		void close() {
			if (in) {
				in->close_port();
				in.reset();
			}
		}

	private:
		MidiListener &listener;
		std::unique_ptr<libremidi::midi_in> in;

		// on the backend's input thread
		void received(const libremidi::message &m) {
			// nobody left to tell while we're being destroyed
			auto self = weak_from_this().lock();
			if (!self || m.bytes.empty()) return;
			listener.midiReceived(self, MidiMessage(m.bytes.data(), m.bytes.size()), nowNs());
		}
	};

	////////////////////////////////////////////////////////////////////////////////////////////////////

	class Output : public MidiDevice {
	public:
		explicit Output(const std::string &name)
			: MidiDevice(name, Direction::Output) {}

		virtual bool open(const libremidi::output_port &port) = 0;

		// may be called from the audio thread (MIDI clock out)
		virtual void send(const MidiMessage &m) = 0;
	};

#if defined(_WIN32)

	// WinMM output of our own rather than libremidi's: its sysex send (like RtMidi's) waits for the driver
	// to finish with each buffer, polling with Sleep(1), which is ~15ms a message on Windows' default
	// timer - an SP-404MKII page change sends 63 of them.
	class WinMmOutput final : public Output {
	public:
		using Output::Output;

		~WinMmOutput() override { close(); }

		bool open(const libremidi::output_port &port) override {
			const auto result = midiOutOpen(&handle, static_cast<UINT>(port.port), 0, 0, CALLBACK_NULL);
			if (result != MMSYSERR_NOERROR) {
				Log::e() << "Failed to open MIDI output '" << name << "': " << describe(result);
				handle = nullptr;
				return false;
			}
			return true;
		}

		void send(const MidiMessage &m) override {
			const auto bytes = m.getBytes();
			if (bytes.empty()) return;
			if (bytes[0] == MidiMessageConstants::MIDI_SYSEX) {
				sendLong(bytes);
			} else {
				sendShort(bytes);
			}
		}

	private:
		struct LongMessage {
			MIDIHDR header {};
			std::vector<char> data;
		};

		HMIDIOUT handle = nullptr;

		// Sysex buffers the driver hasn't handed back yet. Locked by sysex sends only, so short messages
		// (the audio thread's) never wait on it.
		std::mutex longMutex;
		std::vector<std::unique_ptr<LongMessage>> inFlight;

		static std::string describe(MMRESULT result) {
			char text[MAXERRORLENGTH] {};
			midiOutGetErrorTextA(result, text, MAXERRORLENGTH);
			return text;
		}

		void sendShort(const std::vector<uint8_t> &bytes) {
			DWORD packed = 0;
			for (size_t i = 0; i < std::min<size_t>(bytes.size(), 3); i++) {
				packed |= static_cast<DWORD>(bytes[i]) << (8 * i);
			}
			if (const auto result = midiOutShortMsg(handle, packed); result != MMSYSERR_NOERROR) {
				Log::e() << "Failed to send MIDI to '" << name << "': " << describe(result);
			}
		}

		// midiOutLongMsg returns straight away and the driver keeps the buffer until it's sent it, so keep
		// it until the driver's done with it instead of waiting for that here.
		void sendLong(const std::vector<uint8_t> &bytes) {
			std::lock_guard lock(longMutex);
			releaseFinished();

			auto message = std::make_unique<LongMessage>();
			message->data.assign(std::begin(bytes), std::end(bytes));
			message->header.lpData			= message->data.data();
			message->header.dwBufferLength	= static_cast<DWORD>(message->data.size());
			message->header.dwBytesRecorded = static_cast<DWORD>(message->data.size());

			if (const auto result = midiOutPrepareHeader(handle, &message->header, sizeof(MIDIHDR));
				result != MMSYSERR_NOERROR) {
				Log::e() << "Failed to prepare sysex for '" << name << "': " << describe(result);
				return;
			}
			if (const auto result = midiOutLongMsg(handle, &message->header, sizeof(MIDIHDR));
				result != MMSYSERR_NOERROR) {
				Log::e() << "Failed to send sysex to '" << name << "': " << describe(result);
				midiOutUnprepareHeader(handle, &message->header, sizeof(MIDIHDR));
				return;
			}
			inFlight.push_back(std::move(message));
		}

		// The driver finishes them in order, so stop at the first one it's still sending
		void releaseFinished() {
			auto it = std::begin(inFlight);
			while (it != std::end(inFlight)
				   && midiOutUnprepareHeader(handle, &(*it)->header, sizeof(MIDIHDR)) != MIDIERR_STILLPLAYING) {
				++it;
			}
			inFlight.erase(std::begin(inFlight), it);
		}

		void close() {
			if (handle == nullptr) return;
			// hands back everything still queued, so the headers can be released
			midiOutReset(handle);
			{
				std::lock_guard lock(longMutex);
				for (auto &message: inFlight) {
					midiOutUnprepareHeader(handle, &message->header, sizeof(MIDIHDR));
				}
				inFlight.clear();
			}
			midiOutClose(handle);
			handle = nullptr;
		}
	};

#else

	class LibremidiOutput final : public Output {
	public:
		using Output::Output;

		~LibremidiOutput() override {
			if (out) out->close_port();
		}

		bool open(const libremidi::output_port &port) override {
			libremidi::output_configuration conf;
			conf.on_error = [this](std::string_view text, const auto &) {
				Log::e() << "MIDI output '" << name << "': " << text;
			};
			conf.on_warning = [this](std::string_view text, const auto &) {
				Log::w() << "MIDI output '" << name << "': " << text;
			};
			out = std::make_unique<libremidi::midi_out>(conf, outputApi());
			if (auto err = out->open_port(port, clientName); failed(err)) {
				Log::e() << "Failed to open MIDI output '" << name << "': " << describe(err);
				out.reset();
				return false;
			}
			return true;
		}

		void send(const MidiMessage &m) override {
			const auto bytes = m.getBytes();
			if (bytes.empty()) return;
			if (auto err = out->send_message(bytes.data(), bytes.size()); failed(err)) {
				Log::e() << "Failed to send MIDI to '" << name << "': " << describe(err);
			}
		}

	private:
		std::unique_ptr<libremidi::midi_out> out;
	};

#endif

	static std::shared_ptr<Output> makeOutput(const std::string &name) {
#if defined(_WIN32)
		return std::make_shared<WinMmOutput>(name);
#else
		return std::make_shared<LibremidiOutput>(name);
#endif
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////

	// Lists the system's ports and says when they've changed. On Linux libremidi's ALSA observer listens
	// to the sequencer's announce port. libremidi's WinMM observer can only poll, so on Windows we ask
	// for device interface arrivals/removals instead.
	class PortWatcher {
	public:
		explicit PortWatcher(std::function<void()> onChange)
			: onChange(std::move(onChange))
			, observer(configuration(this->onChange), observerApi()) {
#if defined(_WIN32)
			CM_NOTIFY_FILTER filter {};
			filter.cbSize	  = sizeof(filter);
			filter.Flags	  = CM_NOTIFY_FILTER_FLAG_ALL_INTERFACE_CLASSES;
			filter.FilterType = CM_NOTIFY_FILTER_TYPE_DEVICEINTERFACE;
			if (const auto result = CM_Register_Notification(&filter, this, &deviceChanged, &notification);
				result != CR_SUCCESS) {
				Log::e() << "Couldn't register for device change notifications (" << result
						 << "), MIDI devices will only be picked up by the slow rescan";
				notification = nullptr;
			}
#endif
		}

		~PortWatcher() {
#if defined(_WIN32)
			// waits for any callback in progress
			if (notification != nullptr) CM_Unregister_Notification(notification);
#endif
		}

		[[nodiscard]] std::vector<libremidi::input_port> inputs() const {
			return observer.get_input_ports();
		}
		[[nodiscard]] std::vector<libremidi::output_port> outputs() const {
			return observer.get_output_ports();
		}

	private:
		std::function<void()> onChange;
		libremidi::observer observer;

		static libremidi::observer_configuration configuration(const std::function<void()> &onChange) {
			libremidi::observer_configuration conf;
			conf.on_error		= [](std::string_view text, const auto &) { Log::e() << "MIDI: " << text; };
			conf.on_warning		= [](std::string_view text, const auto &) { Log::w() << "MIDI: " << text; };
			conf.track_hardware = true;
			conf.track_virtual	= true;
#if !defined(_WIN32)
			conf.notify_in_constructor = false;
			conf.input_added		   = [onChange](const libremidi::input_port &) { onChange(); };
			conf.input_removed		   = [onChange](const libremidi::input_port &) { onChange(); };
			conf.output_added		   = [onChange](const libremidi::output_port &) { onChange(); };
			conf.output_removed		   = [onChange](const libremidi::output_port &) { onChange(); };
#endif
			return conf;
		}

		static libremidi::observer_api_configuration observerApi() {
#if defined(_WIN32)
			return libremidi::winmm_observer_configuration {};
#else
			libremidi::alsa_seq::observer_configuration conf;
			conf.client_name = clientName;
			return conf;
#endif
		}

#if defined(_WIN32)
		HCMNOTIFICATION notification = nullptr;

		static DWORD CALLBACK
			deviceChanged(HCMNOTIFICATION, PVOID context, CM_NOTIFY_ACTION action, PCM_NOTIFY_EVENT_DATA, DWORD) {
			if (action == CM_NOTIFY_ACTION_DEVICEINTERFACEARRIVAL
				|| action == CM_NOTIFY_ACTION_DEVICEINTERFACEREMOVAL) {
				static_cast<PortWatcher *>(context)->onChange();
			}
			return ERROR_SUCCESS;
		}
#endif
	};

	// Names ports by their own name, numbered only when several have the same one, so a device keeps its
	// name when others come and go
	template <typename Port>
	static std::vector<std::string> uniqueNames(const std::vector<Port> &ports) {
		std::vector<std::string> names;
		for (auto &port: ports) {
			const auto sameName = [&](const Port &other) { return other.port_name == port.port_name; };
			if (std::count_if(std::begin(ports), std::end(ports), sameName) == 1) {
				names.push_back(port.port_name);
			} else {
				const auto before = std::count_if(&ports.front(), &port, sameName);
				names.push_back(port.port_name + " " + std::to_string(before + 1));
			}
		}
		return names;
	}

	// Brings ports in line with the system's current list. Returns whether anything changed.
	template <typename Port, typename Device, typename Open>
	static bool update(std::map<std::string, std::shared_ptr<Device>> &devices,
					   const std::vector<Port> &ports,
					   Open &&open,
					   std::vector<std::shared_ptr<Device>> &removed) {
		bool changed	 = false;
		const auto names = uniqueNames(ports);
		for (size_t i = 0; i < ports.size(); i++) {
			if (isIgnored(ports[i]) || devices.find(names[i]) != std::end(devices)) continue;
			// new midi port! Record it even if it won't open so we don't retry it every rescan.
			Log::d() << "Added MIDI port " << names[i];
			devices[names[i]] = open(names[i], ports[i]);
			changed			  = true;
		}

		for (auto it = std::begin(devices); it != std::end(devices);) {
			if (std::find(std::begin(names), std::end(names), it->first) == std::end(names)) {
				Log::d() << "Removing MIDI port " << it->first;
				removed.push_back(it->second);
				it		= devices.erase(it);
				changed = true;
			} else {
				++it;
			}
		}
		return changed;
	}

} // namespace mzgl::midi

using namespace mzgl::midi;

////////////////////////////////////////////////////////////////////////////////////////////////////

AllMidiDevicesLibremidiImpl::AllMidiDevicesLibremidiImpl() = default;

AllMidiDevicesLibremidiImpl::~AllMidiDevicesLibremidiImpl() {
	if (worker.joinable()) {
		{
			std::lock_guard lock(workerMutex);
			stopRequested = true;
		}
		workerWake.notify_all();
		worker.join();
	}
	// no more change notifications, then close everything
	watcher.reset();
	inputs.clear();
	outputs.clear();
	retiredOutputs.clear();
}

void AllMidiDevicesLibremidiImpl::setup() {
	if (worker.joinable()) return;
	worker = std::thread([this]() { run(); });
}

void AllMidiDevicesLibremidiImpl::requestRescan() {
	{
		std::lock_guard lock(workerMutex);
		rescanRequested = true;
	}
	workerWake.notify_all();
}

void AllMidiDevicesLibremidiImpl::run() {
	watcher = std::make_unique<PortWatcher>([this]() { requestRescan(); });

	// a device change arrives as a burst of notifications, and WinMM can list a new device a moment
	// after its interface arrives, so collect the burst, rescan, and look again shortly after
	static constexpr auto settleTime = 250ms;
	static constexpr auto followUp	 = 1500ms;
	// in case a change goes unnoticed
	static constexpr auto fallback = 5s;

	std::chrono::milliseconds timeout = fallback;
	std::unique_lock lock(workerMutex);
	while (true) {
		workerWake.wait_for(lock, timeout, [this]() { return stopRequested || rescanRequested; });
		if (stopRequested) break;

		const bool changed = rescanRequested;
		if (changed) {
			if (workerWake.wait_for(lock, settleTime, [this]() { return stopRequested; })) break;
			rescanRequested = false;
		}

		lock.unlock();
		rescan();
		lock.lock();

		timeout = changed ? followUp : fallback;
	}
}

void AllMidiDevicesLibremidiImpl::rescan() {
	std::vector<std::shared_ptr<Input>> removedInputs;
	const bool inputsChanged = update(
		inputs,
		watcher->inputs(),
		[this](const std::string &name, const libremidi::input_port &port) {
			auto input = std::make_shared<Input>(name, *this);
			input->open(port);
			return input;
		},
		removedInputs);
	if (inputsChanged) {
		auto snapshot = std::make_shared<std::vector<std::shared_ptr<Input>>>();
		for (auto &input: inputs) {
			snapshot->push_back(input.second);
		}
		inputSnapshot.store(std::move(snapshot));
	}

	std::vector<std::shared_ptr<Output>> removedOutputs;
	const bool outputsChanged = update(
		outputs,
		watcher->outputs(),
		[](const std::string &name, const libremidi::output_port &port) -> std::shared_ptr<Output> {
			auto output = makeOutput(name);
			// only list outputs we could open, so nobody sends to a closed port
			return output->open(port) ? output : nullptr;
		},
		removedOutputs);
	if (outputsChanged) {
		auto snapshot = std::make_shared<std::vector<std::shared_ptr<Output>>>();
		for (auto &output: outputs) {
			if (output.second) snapshot->push_back(output.second);
		}
		outputSnapshot.store(std::move(snapshot));
	}

	// close retired outputs once nobody else holds them (a sender may still have the old snapshot), and
	// keep this rescan's for later
	retiredOutputs.erase(std::remove_if(std::begin(retiredOutputs),
										std::end(retiredOutputs),
										[&](auto &&output) { return output.use_count() == 1; }),
						 std::end(retiredOutputs));
	for (auto &output: removedOutputs) {
		if (output) retiredOutputs.push_back(output);
	}

	if (inputsChanged || outputsChanged) {
		requestAnnouncement();
	}
}

void AllMidiDevicesLibremidiImpl::setMainThreadRunner(MainThreadRunner *runner) {
	{
		std::lock_guard lock(runnerMutex);
		mainThreadRunner = runner;
	}
	requestAnnouncement();
}

// Asks the main thread to bring announcedSnapshot up to date with what the worker has found.
void AllMidiDevicesLibremidiImpl::requestAnnouncement() {
	std::lock_guard lock(runnerMutex);
	if (mainThreadRunner == nullptr) return;
	mainThreadRunner->runOnMainThread([weakSelf = weak_from_this()]() {
		if (auto self = weakSelf.lock()) {
			self->announceChanges();
		}
	});
}

// One device per notification, like CoreMIDI, so a listener that looks for a device's input and output
// together (the SP-404, the Launchpad) sees it complete only once.
void AllMidiDevicesLibremidiImpl::announceChanges() {
	std::vector<std::shared_ptr<MidiDevice>> current;
	for (auto &input: *inputSnapshot.load()) {
		current.push_back(input);
	}
	for (auto &output: *outputSnapshot.load()) {
		current.push_back(output);
	}

	auto contains = [](const std::vector<std::shared_ptr<MidiDevice>> &devices,
					   const std::shared_ptr<MidiDevice> &device) {
		return std::find(std::begin(devices), std::end(devices), device) != std::end(devices);
	};

	auto announced = *announcedSnapshot.load();
	for (auto &device: std::vector(announced)) {
		if (contains(current, device)) continue;
		announced.erase(std::find(std::begin(announced), std::end(announced), device));
		announcedSnapshot.store(std::make_shared<Announced>(announced));
		notifyDisconnection(device);
	}
	for (auto &device: current) {
		if (contains(announced, device)) continue;
		announced.push_back(device);
		announcedSnapshot.store(std::make_shared<Announced>(announced));
		notifyConnection(device);
	}
}

std::vector<std::shared_ptr<MidiDevice>> AllMidiDevicesLibremidiImpl::getConnectedMidiDevices() const {
	return *announcedSnapshot.load();
}

void AllMidiDevicesLibremidiImpl::sendMessage(const MidiMessage &m) {
	for (auto &output: *outputSnapshot.load()) {
		output->send(m);
	}
}

void AllMidiDevicesLibremidiImpl::sendMessage(const std::shared_ptr<MidiDevice> &device,
											  const MidiMessage &m,
											  std::optional<uint64_t> delayInNanoSeconds) {
	for (auto &output: *outputSnapshot.load()) {
		if (*output == *device) {
			output->send(m);
			return;
		}
	}
}

void AllMidiDevicesLibremidiImpl::sendMessageToMatching(
	const MidiMessage &m,
	std::optional<uint64_t> delayInNanoSeconds,
	const std::function<bool(const MidiDevice &)> &shouldSendTo) {
	for (auto &output: *outputSnapshot.load()) {
		if (shouldSendTo(*output)) {
			output->send(m);
		}
	}
}

void AllMidiDevicesLibremidiImpl::midiReceived(const std::shared_ptr<MidiDevice> &device,
											   const MidiMessage &m,
											   uint64_t timestamp) {
	for (auto *listener: listeners) {
		listener->midiReceived(device, m, timestamp);
	}
}
