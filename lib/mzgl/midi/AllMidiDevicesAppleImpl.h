//
//  AllMidiDevicesApple.h
//  mzgl
//
//  Created by Marek Bereza on 13/11/2021.
//  Copyright © 2021 Marek Bereza. All rights reserved.
//

#pragma once
#include <vector>
#include <string>
#include <thread>

#include <CoreMIDI/CoreMIDI.h>
#include "MidiMessage.h"
#include "Midi.h"
#include "AllMidiDevicesImpl.h"
class MidiDevice {
public:
    MIDIEndpointRef endpoint;
    std::string name;
    bool isNetworkSession = false;
    MidiDevice(MIDIEndpointRef endpoint) : endpoint(endpoint) {}
};

class MidiSource : public MidiDevice {
public:
    static std::shared_ptr<MidiSource> create(MIDIEndpointRef endpoint) {
        return std::shared_ptr<MidiSource>(new MidiSource(endpoint));
    }

private:
    MidiSource(MIDIEndpointRef endpoint) : MidiDevice(endpoint)  {}
};

class MidiDestination : public MidiDevice {
public:
    static std::shared_ptr<MidiDestination> create(MIDIEndpointRef endpoint) {
        return std::shared_ptr<MidiDestination>(new MidiDestination(endpoint));
    }

private:
    MidiDestination(MIDIEndpointRef endpoint) : MidiDevice(endpoint) {}
};


typedef std::shared_ptr<MidiDevice> MidiDeviceRef;
typedef std::shared_ptr<MidiSource> MidiSourceRef;
typedef std::shared_ptr<MidiDestination> MidiDestinationRef;


class MidiDeviceListener {
public:
    virtual void midiDevicesChanged() {}
};


class AllMidiDevicesAppleImpl : public AllMidiDevicesImpl {
public:

    std::vector<MidiDeviceListener*> deviceListeners;

    MIDIClientRef client = 0;
    MIDIPortRef inputPort = 0;
    MIDIPortRef outputPort = 0;
    MIDIEndpointRef    virtualDestinationEndpoint = 0;
    MIDIEndpointRef    virtualSourceEndpoint = 0;

    std::vector<MidiSourceRef> sources;
    std::vector<MidiDestinationRef> destinations;
    std::vector<MidiListener*> listeners;
    
    void setup() override;

    void addListener(MidiListener *l) override {
        listeners.push_back(l);
    }

    void scanForDevices();
    
    
    void midiReceived(const MidiMessage &msg, uint64_t timestamp);

    void sendMessage(const MidiMessage &m) override;
	
    virtual ~AllMidiDevicesAppleImpl();
    

    // needs to be public because it's called by callbacks
    void packetListReceived(const MIDIPacketList *packetList);
    void midiNotify(const MIDINotification *notification);
    
private:
    std::atomic<bool> running { false };
    std::thread portScannerThread;
    void autoPoll();
    void notifyChange() {
        for(auto *l : deviceListeners) {
            l->midiDevicesChanged();
        }
    }

    void connectDestination(MIDIEndpointRef endpoint);
    void disconnectDestination(MIDIEndpointRef endpoint);

    void connectSource(MIDIEndpointRef endpoint);
    void disconnectSource(MIDIEndpointRef endpoint);
    MidiSourceRef getSource(MIDIEndpointRef endpoint);
    MidiDestinationRef getDestination(MIDIEndpointRef endpoint);


    std::vector<unsigned char> pendingMsg;

    void midiNotifyAdd(const MIDIObjectAddRemoveNotification *notification);
    void midiNotifyRemove(const MIDIObjectAddRemoveNotification *notification);

    void sendPacketList(const MIDIPacketList *packetList);
    void sendBytes(const UInt8 *data, UInt32 size);
};
