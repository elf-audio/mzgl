//
//  AllMidiDevicesImpl.h
//  mzgl
//
//  Created by Marek Bereza on 13/11/2021.
//  Copyright © 2021 Marek Bereza. All rights reserved.
//

#pragma once
#include "AllMidiDevices.h"
#include "Listenable.h"

class AllMidiDevicesImpl : public Listenable<MidiListener> {
public:
	~AllMidiDevicesImpl() override = default;

	virtual std::vector<std::shared_ptr<MidiDevice>> getConnectedMidiDevices() const = 0;
	virtual void setup() {}

	// send message to all connected midi devices that have an input
	virtual void sendMessage(const MidiMessage &m) = 0;

	// send message to a specific device
	virtual void sendMessage(const std::shared_ptr<MidiDevice> &device,
							 const MidiMessage &m,
							 std::optional<uint64_t> delayInNanoSeconds) = 0;

	// send message to every connected device that shouldSendTo() accepts. Implementations that can
	// walk their device list in place override this so a send doesn't build a list first
	virtual void sendMessageToMatching(const MidiMessage &m,
									   std::optional<uint64_t> delayInNanoSeconds,
									   const std::function<bool(const MidiDevice &)> &shouldSendTo) {
		for (auto &device: getConnectedMidiDevices()) {
			if (shouldSendTo(*device)) {
				sendMessage(device, m, delayInNanoSeconds);
			}
		}
	}

	void addConnectionListener(MidiConnectionListener *listener) { connectionListeners.push_back(listener); }
	void removeConnectionListener(MidiConnectionListener *listener) {
		connectionListeners.erase(std::remove(connectionListeners.begin(), connectionListeners.end(), listener),
								  connectionListeners.end());
	}

protected:
	void notifyConnection(const std::shared_ptr<MidiDevice> &dev) {
		for (auto *listener: connectionListeners) {
			listener->midiDeviceConnected(dev);
		}
	}

	void notifyDisconnection(const std::shared_ptr<MidiDevice> &dev) {
		for (auto *listener: connectionListeners) {
			listener->midiDeviceDisconnected(dev);
		}
	}

private:
	std::vector<MidiConnectionListener *> connectionListeners;
};
