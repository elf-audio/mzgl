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

void removeDeviceFromDeviceList(const std::shared_ptr<AndroidMidiDevice> &device,
								std::vector<std::shared_ptr<AndroidMidiDevice>> &devices) {
	devices.erase(std::remove_if(std::begin(devices),
								 std::end(devices),
								 [&](auto &&otherDevice) { return *device == *otherDevice; }),
				  std::end(devices));
}

void AllMidiDevicesAndroidImpl::connect(const std::shared_ptr<AndroidMidiDevice> &device,
										std::vector<std::shared_ptr<AndroidMidiDevice>> &devices) {
	mzAssert(mainThreadRunner != nullptr && mainThreadRunner->isMainThread());
	if (std::find_if(
			std::cbegin(devices), std::cend(devices), [&](auto &&otherDevice) { return *otherDevice == *device; })
		== std::cend(devices)) {
		devices.push_back(device);
		notifyConnection(device);
	}
}

void AllMidiDevicesAndroidImpl::disconnect(const std::shared_ptr<AndroidMidiDevice> &device,
										   std::vector<std::shared_ptr<AndroidMidiDevice>> &devices) {
	mzAssert(mainThreadRunner != nullptr && mainThreadRunner->isMainThread());
	removeDeviceFromDeviceList(device, devices);
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
	for (auto &output: midiOutputs) {
		sendMessage(output, message);
	}
}

void AllMidiDevicesAndroidImpl::sendMessage(const std::shared_ptr<MidiDevice> &device,
											const MidiMessage &message) {
	if (auto theDevice = dynamic_cast<AndroidMidiDevice *>(device.get())) {
		for (auto output: midiOutputs) {
			if (*output == *theDevice) {
				androidSendMidi(message.getBytes(), theDevice->deviceIdentifier, theDevice->devicePort);
			}
		}
	}
}

std::vector<std::shared_ptr<MidiDevice>> AllMidiDevicesAndroidImpl::getConnectedMidiDevices() const {
	std::vector<std::shared_ptr<MidiDevice>> devices;
	devices.insert(std::end(devices), std::begin(midiInputs), std::end(midiInputs));
	devices.insert(std::end(devices), std::begin(midiOutputs), std::end(midiOutputs));
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