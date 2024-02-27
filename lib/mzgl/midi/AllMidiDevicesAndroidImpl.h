//
// Created by Marek Bereza on 26/02/2024.
//

#pragma once
#include "AllMidiDevicesImpl.h"
#include "androidUtil.h"

class AllMidiDevicesAndroidImpl
	: public AllMidiDevicesImpl
	, std::enable_shared_from_this<AllMidiDevicesAndroidImpl> {
public:
	void setup() override { androidSetupAllMidiIns(weak_from_this()); }

	void messageReceived(const MidiDevice &device, const MidiMessage &m, uint64_t timestamp) {
		for (auto l: listeners)
			l->midiReceived(device, m, timestamp);
	}

	void sendMessage(const MidiMessage &m) override {
		Log::e() << "AllMidiDevicesAndroidImpl::sendMessage() Unimplemented";
	}

	void sendMessage(const MidiDevice &device, const MidiMessage &m) override {
		Log::e() << "AllMidiDevicesAndroidImpl::sendMessage() Unimplemented";
	}

	std::vector<MidiDevice> getConnectedMidiDevices() override {
		Log::e() << "AllMidiDevicesAndroidImpl::getConnectedMidiDevices() unimplemented";
		return {};
	}
};