//
// Created by Marek Bereza on 26/02/2024.
//

#pragma once

#include "androidUtil.h"

class AllMidiDevicesAndroidImpl : public AllMidiDevicesImpl {
	void setup() override { androidSetupAllMidiIns(); }

	void addListener(MidiListener *listener) override { androidAddMidiListener(listener); }
	void removeListener(MidiListener *listener) override { androidRemoveMidiListener(listener); }

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