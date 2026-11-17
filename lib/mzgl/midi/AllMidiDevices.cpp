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
#include "mainThread.h"

#ifdef __APPLE__
#	include "AllMidiDevicesAppleImpl.h"
#elif defined(__ANDROID__)
#	include "AllMidiDevicesAndroidImpl.h"
#else
#	include "AllMidiDevicesLibremidiImpl.h"
#endif

AllMidiDevices::AllMidiDevices(bool online)
	: online(online) {
	if (online) {
#if defined(__ANDROID__)
		impl = std::make_shared<AllMidiDevicesAndroidImpl>();
#elif defined(__APPLE__)
		impl = std::make_shared<AllMidiDevicesAppleImpl>();
#else
		impl = std::make_shared<AllMidiDevicesLibremidiImpl>();
#endif
	}
}

void AllMidiDevices::setMainThreadRunner(MainThreadRunner *runner) {
#if defined(__ANDROID__)
	if (auto ptr = dynamic_cast<AllMidiDevicesAndroidImpl *>(impl.get())) {
		ptr->setMainThreadRunner(runner);
	}
#elif !defined(__APPLE__)
	if (auto ptr = dynamic_cast<AllMidiDevicesLibremidiImpl *>(impl.get())) {
		ptr->setMainThreadRunner(runner);
	}
#endif
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

std::vector<std::shared_ptr<MidiDevice>> AllMidiDevices::getConnectedMidiDevices() const {
	if (online) {
		return impl->getConnectedMidiDevices();
	}

	return {};
}

void AllMidiDevices::sendMessage(const std::shared_ptr<MidiDevice> &device,
								 const MidiMessage &m,
								 std::optional<uint64_t> delayInNanoSeconds) {
	if (online) {
		impl->sendMessage(device, m, delayInNanoSeconds);
	}
}
void AllMidiDevices::sendMessageToMatching(const MidiMessage &m,
										   std::optional<uint64_t> delayInNanoSeconds,
										   const std::function<bool(const MidiDevice &)> &shouldSendTo) {
	if (online) {
		impl->sendMessageToMatching(m, delayInNanoSeconds, shouldSendTo);
	}
}
void AllMidiDevices::removeListener(MidiListener *listener) {
	if (online) {
		impl->removeListener(listener);
	}
}