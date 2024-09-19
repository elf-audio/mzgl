#pragma once

#include "AllMidiDevicesImpl.h"
#include "androidUtil.h"

using AndroidMidiIdentifier = int;

class AndroidMidiDevice : public MidiDevice {
public:
	enum class Type { input, output };
	AndroidMidiDevice(const std::string &deviceName,
					  AndroidMidiIdentifier device,
					  AndroidMidiIdentifier port,
					  Type type)
		: MidiDevice(deviceName, type == Type::output)
		, deviceIdentifier(device)
		, devicePort(port) {}

	AndroidMidiDevice(const AndroidMidiDevice &other) { *this = other; }

	AndroidMidiDevice &operator=(const AndroidMidiDevice &other) {
		if (this != &other) {
			name			 = other.name;
			isOutput		 = other.isOutput;
			deviceIdentifier = other.deviceIdentifier;
			devicePort		 = other.devicePort;
			id				 = other.id;
		}
		return *this;
	}

	~AndroidMidiDevice() override = default;

	[[nodiscard]] bool operator==(const AndroidMidiDevice &other) const {
		return std::tie(deviceIdentifier, devicePort, isOutput)
			   == std::tie(other.deviceIdentifier, other.devicePort, other.isOutput);
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
					 std::optional<uint64_t> timeStampInNanoSeconds = std::nullopt) override;
	std::vector<std::shared_ptr<MidiDevice>> getConnectedMidiDevices() const override;

private:
	void connect(const std::shared_ptr<AndroidMidiDevice> &device,
				 std::vector<std::shared_ptr<AndroidMidiDevice>> &devices);
	void disconnect(const std::shared_ptr<AndroidMidiDevice> &device,
					std::vector<std::shared_ptr<AndroidMidiDevice>> &devices);

	void runOnMainThread(const std::function<void(AllMidiDevicesAndroidImpl &)> &fun);

	std::vector<std::shared_ptr<AndroidMidiDevice>> midiInputs;
	std::vector<std::shared_ptr<AndroidMidiDevice>> midiOutputs;
	MainThreadRunner *mainThreadRunner {nullptr};
};