#include "AllMidiDevicesAndroidImpl.h"

void AllMidiDevicesAndroidImpl::setMainThreadRunner(MainThreadRunner *runner) {
	mainThreadRunner = runner;
}

void AllMidiDevicesAndroidImpl::setup() {
	androidSetupMidiManager(shared_from_this());
}

void AllMidiDevicesAndroidImpl::messageReceived(const std::shared_ptr<MidiDevice> &device,
												const MidiMessage &message,
												uint64_t timestamp) {
	for (auto listener: listeners) {
		listener->midiReceived(device, message, timestamp);
	}
}

AllMidiDevicesAndroidImpl::DevicesSnapshot AllMidiDevicesAndroidImpl::load(const DevicesSnapshot &devices) {
	return std::atomic_load(&devices);
}

void AllMidiDevicesAndroidImpl::connect(const std::shared_ptr<AndroidMidiDevice> &device,
										DevicesSnapshot &devices) {
	mzAssert(mainThreadRunner != nullptr && mainThreadRunner->isMainThread());
	const auto current = load(devices);
	if (std::find_if(std::cbegin(*current),
					 std::cend(*current),
					 [&](auto &&otherDevice) { return *otherDevice == *device; })
		== std::cend(*current)) {
		auto updated = std::make_shared<Devices>(*current);
		updated->push_back(device);
		std::atomic_store(&devices, DevicesSnapshot {std::move(updated)});
		notifyConnection(device);
	}
}

void AllMidiDevicesAndroidImpl::disconnect(const std::shared_ptr<AndroidMidiDevice> &device,
										   DevicesSnapshot &devices) {
	mzAssert(mainThreadRunner != nullptr && mainThreadRunner->isMainThread());
	auto updated = std::make_shared<Devices>(*load(devices));
	updated->erase(std::remove_if(std::begin(*updated),
								  std::end(*updated),
								  [&](auto &&otherDevice) { return *device == *otherDevice; }),
				   std::end(*updated));
	std::atomic_store(&devices, DevicesSnapshot {std::move(updated)});
	notifyDisconnection(device);
}

void AllMidiDevicesAndroidImpl::inputDeviceConnected(const std::shared_ptr<AndroidMidiDevice> &device) {
	runOnMainThread([device](AllMidiDevicesAndroidImpl &impl) { impl.connect(device, impl.midiInputs); });
}

void AllMidiDevicesAndroidImpl::outputDeviceConnected(const std::shared_ptr<AndroidMidiDevice> &device) {
	runOnMainThread([device](AllMidiDevicesAndroidImpl &impl) { impl.connect(device, impl.midiOutputs); });
}

void AllMidiDevicesAndroidImpl::inputDeviceDisconnected(const std::shared_ptr<AndroidMidiDevice> &device) {
	runOnMainThread([device](AllMidiDevicesAndroidImpl &impl) { impl.disconnect(device, impl.midiInputs); });
}

void AllMidiDevicesAndroidImpl::outputDeviceDisconnected(const std::shared_ptr<AndroidMidiDevice> &device) {
	runOnMainThread([device](AllMidiDevicesAndroidImpl &impl) { impl.disconnect(device, impl.midiOutputs); });
}

void AllMidiDevicesAndroidImpl::sendMessage(const MidiMessage &message) {
	const auto outputs = load(midiOutputs);
	for (auto &output: *outputs) {
		sendMessage(output, message, std::nullopt);
	}
}

void AllMidiDevicesAndroidImpl::sendMessage(const std::shared_ptr<MidiDevice> &device,
											const MidiMessage &message,
											std::optional<uint64_t> timeStampInNanoSeconds) {
	if (auto theDevice = dynamic_cast<AndroidMidiDevice *>(device.get())) {
		const auto outputs = load(midiOutputs);
		for (auto &output: *outputs) {
			if (*output == *theDevice) {
				androidSendMidi(message.getBytes(), theDevice->deviceIdentifier, theDevice->devicePort, timeStampInNanoSeconds);
			}
		}
	}
}

void AllMidiDevicesAndroidImpl::sendMessageToMatching(
	const MidiMessage &message,
	std::optional<uint64_t> timeStampInNanoSeconds,
	const std::function<bool(const MidiDevice &)> &shouldSendTo) {
	const auto outputs = load(midiOutputs);
	for (auto &output: *outputs) {
		if (shouldSendTo(*output)) {
			androidSendMidi(
				message.getBytes(), output->deviceIdentifier, output->devicePort, timeStampInNanoSeconds);
		}
	}
}

std::shared_ptr<MidiDevice> AllMidiDevicesAndroidImpl::findInputDevice(AndroidMidiIdentifier deviceId,
																	   AndroidMidiIdentifier portId) const {
	const auto inputs = load(midiInputs);
	for (auto &input: *inputs) {
		if (input->deviceIdentifier == deviceId && input->devicePort == portId) {
			return input;
		}
	}
	return nullptr;
}

std::vector<std::shared_ptr<MidiDevice>> AllMidiDevicesAndroidImpl::getConnectedMidiDevices() const {
	const auto inputs  = load(midiInputs);
	const auto outputs = load(midiOutputs);
	std::vector<std::shared_ptr<MidiDevice>> devices;
	devices.insert(std::end(devices), std::begin(*inputs), std::end(*inputs));
	devices.insert(std::end(devices), std::begin(*outputs), std::end(*outputs));
	return devices;
}

void AllMidiDevicesAndroidImpl::runOnMainThread(const std::function<void(AllMidiDevicesAndroidImpl &)> &fun) {
	if (mainThreadRunner == nullptr) {
		return;
	}
	mainThreadRunner->runOnMainThread(true, [weak = weak_from_this(), fun = fun] {
		if (auto self = weak.lock()) {
			fun(*self);
		}
	});
}