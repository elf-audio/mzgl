#pragma once

#include "AllMidiDevicesImpl.h"
#include "androidUtil.h"

using AndroidMidiIdentifier = int;

class AndroidMidiDevice : public MidiDevice {
public:
	AndroidMidiDevice(const std::string &deviceName,
					  AndroidMidiIdentifier device,
					  AndroidMidiIdentifier port,
					  MidiDevice::Direction direction)
		: MidiDevice(deviceName, direction)
		, deviceIdentifier(device)
		, devicePort(port) {}

	AndroidMidiDevice(const AndroidMidiDevice &other) { *this = other; }

	AndroidMidiDevice &operator=(const AndroidMidiDevice &other) {
		if (this != &other) {
			name			 = other.name;
			direction		 = other.direction;
			deviceIdentifier = other.deviceIdentifier;
			devicePort		 = other.devicePort;
			id				 = other.id;
		}
		return *this;
	}

	~AndroidMidiDevice() override = default;

	[[nodiscard]] bool operator==(const AndroidMidiDevice &other) const {
		return std::tie(deviceIdentifier, devicePort, direction)
			   == std::tie(other.deviceIdentifier, other.devicePort, other.direction);
	}

	[[nodiscard]] bool operator==(const MidiDevice &other) const override {
		if (auto android = dynamic_cast<const AndroidMidiDevice *>(&other)) {
			return *this == *android;
		}
		return MidiDevice::operator==(other);
	}

	[[nodiscard]] bool operator!=(const AndroidMidiDevice &other) const { return !(*this == other); }

	[[nodiscard]] bool operator!=(const MidiDevice &other) const override { return !(*this == other); }

	AndroidMidiIdentifier deviceIdentifier;
	AndroidMidiIdentifier devicePort;
};

class AllMidiDevicesAndroidImpl
	: public AllMidiDevicesImpl
	, public std::enable_shared_from_this<AllMidiDevicesAndroidImpl> {
public:
	void setup() override;
	void setMainThreadRunner(MainThreadRunner *runner);

	void inputDeviceConnected(const std::shared_ptr<AndroidMidiDevice> &device);
	void outputDeviceConnected(const std::shared_ptr<AndroidMidiDevice> &device);
	void inputDeviceDisconnected(const std::shared_ptr<AndroidMidiDevice> &device);
	void outputDeviceDisconnected(const std::shared_ptr<AndroidMidiDevice> &device);

	void messageReceived(const std::shared_ptr<MidiDevice> &device, const MidiMessage &m, uint64_t timestamp);

	void sendMessage(const MidiMessage &m) override;
	void sendMessage(const std::shared_ptr<MidiDevice> &device,
					 const MidiMessage &m,
					 std::optional<uint64_t> timeStampInNanoSeconds) override;
	std::vector<std::shared_ptr<MidiDevice>> getConnectedMidiDevices() const override;

private:
	using Devices		  = std::vector<std::shared_ptr<AndroidMidiDevice>>;
	using DevicesSnapshot = std::shared_ptr<const Devices>;

	// call on the main thread
	void connect(const std::shared_ptr<AndroidMidiDevice> &device, DevicesSnapshot &devices);
	void disconnect(const std::shared_ptr<AndroidMidiDevice> &device, DevicesSnapshot &devices);
	[[nodiscard]] static DevicesSnapshot load(const DevicesSnapshot &devices);

	void runOnMainThread(const std::function<void(AllMidiDevicesAndroidImpl &)> &fun);

	// the main thread swaps in a new list on connect/disconnect; MIDI receive threads (one per Android
	// port) and senders read whichever list was current, through load(). They used to share one vector
	// that the main thread push_back'd and erased under them
	DevicesSnapshot midiInputs {std::make_shared<const Devices>()};
	DevicesSnapshot midiOutputs {std::make_shared<const Devices>()};
	MainThreadRunner *mainThreadRunner {nullptr};
};