//
//  AllMidiDevicesApple.cpp
//  mzgl
//
//  Created by Marek Bereza on 13/11/2021.
//  Copyright © 2021 Marek Bereza. All rights reserved.
//

#include "AllMidiDevicesAppleImpl.h"
#include <Foundation/Foundation.h>

#include "appleMidiUtils.h"
#include <memory.h>

void NSLogError(OSStatus c,const std::string &str) {
    if(c!=noErr) {
        printf("Error trying to : %s\n", str.c_str());
    }
    // do {
    //     if (c) NSLog(@"Error (%@): %ld:%@", str, (long)c,[NSError errorWithDomain:NSMachErrorDomain code:c userInfo:nil]);
    // }
    // while(false);
}


static std::string nameOfEndpoint(MIDIEndpointRef ref) {
    CFStringRef string = nil;
    OSStatus s = MIDIObjectGetStringProperty(ref, kMIDIPropertyDisplayName, ( CFStringRef*)&string);
    if ( s != noErr )
    {
        return "Unknown name";
    }
    NSString *str = (NSString*)CFBridgingRelease(string);
    return [str UTF8String];
}

static void MIDIReadNoteProc(const MIDIPacketList *packetList, void *readProcRefCon, void *srcConnRefCon) {
    AllMidiDevicesAppleImpl *me = (AllMidiDevicesAppleImpl*)readProcRefCon;
    me->packetListReceived(packetList);
}

static void myMIDINotifyProc(const MIDINotification *message, void *refCon) {
    AllMidiDevicesAppleImpl *me = (AllMidiDevicesAppleImpl*)refCon;
    me->midiNotify(message);
}



static void removeFirst(std::vector<MidiSourceRef> &devs, MidiSourceRef dev) {
    for(int i = 0; i < devs.size(); i++) {
        if(devs[i]==dev) {
            devs.erase(devs.begin() + i);
            return;
        }
    }
}
static void removeFirst(std::vector<MidiDestinationRef> &devs, MidiDestinationRef dev) {
    for(int i = 0; i < devs.size(); i++) {
        if(devs[i]==dev) {
            devs.erase(devs.begin() + i);
            return;
        }
    }
}

void AllMidiDevicesAppleImpl::setup() {
    dispatch_async(dispatch_get_main_queue(), ^{
        OSStatus s = MIDIClientCreate(CFSTR("koala MIDI Client"), myMIDINotifyProc, this, &client);

        s = MIDIOutputPortCreate(client, CFSTR("koala Output Port"), &outputPort);
        NSLogError(s, "Create output MIDI port");

        s = MIDIInputPortCreate(client, CFSTR("koala Input Port"), MIDIReadNoteProc, this, &inputPort);
        NSLogError(s, "Create input MIDI port");

    //    scanForDevices();
        autoPoll();
        
    });
}

   

void AllMidiDevicesAppleImpl::autoPoll() {
    if(running) return;
    // keep checking for new ports
    running = true;
    portScannerThread = std::thread([this]() {
#if defined(__APPLE__) && DEBUG
				pthread_setname_np("AllMidiIns::portScanner");
#endif
        while(running) {
            runOnMainThread([this] {
                dispatch_async(dispatch_get_main_queue(), ^{
                    scanForDevices();
                });
            });
            for(int i = 0; i < 100; i++) {
                sleepMillis(10);
                if(!running) break;
            }
        }
    });
}


void AllMidiDevicesAppleImpl::scanForDevices() {

    const ItemCount numberOfDestinations = MIDIGetNumberOfDestinations();
    const ItemCount numberOfSources      = MIDIGetNumberOfSources();

    auto removedSources       = sources;
    auto removedDestinations  = destinations;

    for (ItemCount index = 0; index < numberOfDestinations; ++index) {
        MIDIEndpointRef endpoint = MIDIGetDestination(index);
        if (endpoint == virtualDestinationEndpoint) continue;

        bool matched = false;
        for (auto destination : destinations) {
            if (destination->endpoint == endpoint) {
                removeFirst(removedDestinations, destination);
                matched = true;
                break;
            }
        }
        if (matched) continue;
        connectDestination(endpoint);
    }

    for (ItemCount index = 0; index < numberOfSources; ++index) {
        MIDIEndpointRef endpoint = MIDIGetSource(index);
        if (endpoint == virtualSourceEndpoint) continue;
        
        bool matched = false;
        for (auto source : sources) {
            if (source->endpoint == endpoint) {
                removeFirst(removedSources, source);
                matched = true;
                break;
            }
        }
        if (matched) continue;
        connectSource(endpoint);
    }

    for (auto destination : removedDestinations) {
        disconnectDestination(destination->endpoint);
    }
    
    for (auto source : removedSources) {
        disconnectSource(source->endpoint);
    }
}

void AllMidiDevicesAppleImpl::connectDestination(MIDIEndpointRef endpoint) {
    Log::d() << "Connecting destination " << nameOfEndpoint(endpoint);
    auto dest = MidiDestination::create(endpoint);
    destinations.push_back(dest);
    notifyChange();
}


void AllMidiDevicesAppleImpl::disconnectDestination(MIDIEndpointRef endpoint) {
    Log::d() << "Disconnecting destination " << nameOfEndpoint(endpoint);
    auto dest = getDestination(endpoint);
    if(dest) {
        removeFirst(destinations, dest);
        notifyChange();
    }
}




void AllMidiDevicesAppleImpl::connectSource(MIDIEndpointRef endpoint) {
    Log::d() << "Connecting source " << nameOfEndpoint(endpoint);
    auto src = MidiSource::create(endpoint);
    sources.push_back(src);

    notifyChange();
    OSStatus s = MIDIPortConnectSource(inputPort, endpoint, (void*)src.get());
    NSLogError(s, "Connecting to MIDI source");
}

MidiSourceRef AllMidiDevicesAppleImpl::getSource(MIDIEndpointRef endpoint) {
    for(auto s : sources) {
        if(s->endpoint==endpoint) return s;
    }
    return nullptr;
}

MidiDestinationRef AllMidiDevicesAppleImpl::getDestination(MIDIEndpointRef endpoint) {
    for(auto d : destinations) {
        if(d->endpoint==endpoint) return d;
    }
    return nullptr;
}

void AllMidiDevicesAppleImpl::disconnectSource(MIDIEndpointRef endpoint) {
    Log::d() << "Disconnecting source " << nameOfEndpoint(endpoint);
    auto src = getSource(endpoint);
    if(src) {
        OSStatus s = MIDIPortDisconnectSource(inputPort, endpoint);
        NSLogError(s, "Disconnecting from MIDI source");
        // [sources removeObject:source];
        removeFirst(sources, src);
        notifyChange();
    }
}


void AllMidiDevicesAppleImpl::midiReceived(const MidiMessage &msg, uint64_t timestamp) {
    uint64_t ts = hostTicksToNanoSeconds(timestamp);
    for(auto *l : listeners) {
        l->midiReceived(msg, ts);
    }
}


void AllMidiDevicesAppleImpl::packetListReceived(const MIDIPacketList *packetList) {
    // if this message is from coremidi, and audiobus is enabled, ignore it.
    // if this message is from audiobus and it is diabled, ignore it (probs never happens that way round)
    
    
    const MIDIPacket *packet = &packetList->packet[0];
    for (int whichPacket = 0; whichPacket < packetList->numPackets; ++whichPacket) {
        
        for(int i = 0; i < packet->length; i++) {
            auto *d = packet->data;
            //printBinary(d[i]);
            if(d[i]&0x80) {
                // this is a status byte, send any previous messages
                if(pendingMsg.size()>0) {
                    midiReceived(MidiMessage(pendingMsg), packet->timeStamp);
                    pendingMsg.clear();
                }
                
                pendingMsg.push_back(d[i]);
                
                // if this status byte is for a 1 byte message send it straight away
                // all status 0xF6 and above are all the 1 byte messages.
                if(pendingMsg.size()==1 && pendingMsg[0]>=0xF6) {
                    midiReceived(MidiMessage(pendingMsg), packet->timeStamp);
                    pendingMsg.clear();
                }
            } else {
                // a data byte

                // first check we have a status in the gun, otherwise can this message
                if(pendingMsg.size()==0 || (pendingMsg[0]&0x80)==0) {
                    pendingMsg.clear();
                } else {
                    pendingMsg.push_back(d[i]);
                    // now check to see if the message is finished
                    // by seeing which status we have and looking it
                    // up in known statuses/lengths
                    int status = pendingMsg[0] & 0xF0;
                    bool shouldEmit = false;
                    switch(status) {
                        case MIDI_NOTE_OFF:
                        case MIDI_NOTE_ON:
                        case MIDI_PITCH_BEND:
                        case MIDI_POLY_AFTERTOUCH:
                        case MIDI_SONG_POS_POINTER:
                        case MIDI_CONTROL_CHANGE:
                            if(pendingMsg.size()==3) shouldEmit = true; break;
                        case MIDI_PROGRAM_CHANGE:
                        case MIDI_AFTERTOUCH:
                        case MIDI_SONG_SELECT:
                            if(pendingMsg.size()==2) shouldEmit = true; break;
                        default: // status unknown, don't send, next message will push this through
                            break;
                    }
                    if(shouldEmit) {
                        midiReceived(MidiMessage(pendingMsg), packet->timeStamp);
                        pendingMsg.clear();
                    }
                }
            }
        }
        packet = MIDIPacketNext(packet);
    }
}




void AllMidiDevicesAppleImpl::midiNotifyAdd(const MIDIObjectAddRemoveNotification *notification) {
    if (notification->child == virtualDestinationEndpoint || notification->child == virtualSourceEndpoint) return;

    if (notification->childType == kMIDIObjectType_Destination) connectDestination((MIDIEndpointRef)notification->child);
    else if (notification->childType == kMIDIObjectType_Source) connectSource((MIDIEndpointRef)notification->child);
}

void AllMidiDevicesAppleImpl::midiNotifyRemove(const MIDIObjectAddRemoveNotification *notification) {
    if (notification->child == virtualDestinationEndpoint || notification->child == virtualSourceEndpoint) return;

    if (notification->childType == kMIDIObjectType_Destination) disconnectDestination((MIDIEndpointRef)notification->child);
    else if (notification->childType == kMIDIObjectType_Source) disconnectSource((MIDIEndpointRef)notification->child);
}


void AllMidiDevicesAppleImpl::midiNotify(const MIDINotification *notification) {

    switch (notification->messageID) {
        case kMIDIMsgObjectAdded:
            midiNotifyAdd((const MIDIObjectAddRemoveNotification *)notification);
            break;
        case kMIDIMsgObjectRemoved:
            midiNotifyRemove((const MIDIObjectAddRemoveNotification *)notification);
            break;
        case kMIDIMsgSetupChanged:
        case kMIDIMsgPropertyChanged:
        case kMIDIMsgThruConnectionsChanged:
        case kMIDIMsgSerialPortOwnerChanged:
        case kMIDIMsgIOError:
            break;
    }
}


void AllMidiDevicesAppleImpl::sendPacketList(const MIDIPacketList *packetList) {
    for (ItemCount index = 0; index < MIDIGetNumberOfDestinations(); ++index) {
//        printf("sending message\n");

        MIDIEndpointRef outputEndpoint = MIDIGetDestination(index);
        if (outputEndpoint) {
            // Send it
            OSStatus s = MIDISend(outputPort, outputEndpoint, packetList);
            NSLogError(s, "Sending MIDI");
        }
    }
}

void AllMidiDevicesAppleImpl::sendBytes(const UInt8*data, UInt32 size) {
//    NSLog(@"%s(%u bytes to core MIDI)", __func__, unsigned(size));
    assert(size < 65536);
    Byte packetBuffer[size+100];
    MIDIPacketList *packetList = (MIDIPacketList*)packetBuffer;
    MIDIPacket     *packet     = MIDIPacketListInit(packetList);

    packet = MIDIPacketListAdd(packetList, sizeof(packetBuffer), packet, 0, size, data);

    sendPacketList(packetList);
}

/*
 struct MIDISysexSendRequest
 {
     MIDIEndpointRef        destination;
     const Byte *          data;
     UInt32                bytesToSend;
     Boolean                complete;
     Byte                reserved[3];
     MIDICompletionProc     completionProc;
     void * __nullable    completionRefCon;
 };
 */

static void cleanupSysex(MIDISysexSendRequest *request) {
//    delete [] request->data;
//    printf("cleanup %d\n", request->complete);
    delete request;
}

void AllMidiDevicesAppleImpl::sendSysex(const UInt8 *data, UInt32 size) {
//    NSLog(@"%s(%u sysex bytes to core MIDI)", __func__, unsigned(size));
    assert(size < 65536);
    
    
    for (ItemCount index = 0; index < MIDIGetNumberOfDestinations(); ++index) {
        MIDISysexSendRequest *request = new MIDISysexSendRequest();
        
        request->data = new Byte[size];
        memcpy((void*)request->data, (void*)data, size);
        request->bytesToSend = size;
        request->completionProc = &cleanupSysex;
        request->completionRefCon = request;
        request->complete = false;
        request->destination = MIDIGetDestination(index);
        OSStatus res = MIDISendSysex( request);
        if(res!=noErr) {
            printf("error sending sysex\n");
        }
    }


}

AllMidiDevicesAppleImpl::~AllMidiDevicesAppleImpl() {
    if (outputPort) {
        OSStatus s = MIDIPortDispose(outputPort);
        NSLogError(s, "Dispose MIDI port");
    }

    if (inputPort) {
        OSStatus s = MIDIPortDispose(inputPort);
        NSLogError(s, "Dispose MIDI port");
    }

    if (client) {
        OSStatus s = MIDIClientDispose(client);
        NSLogError(s, "Dispose MIDI client");
    }
}

void AllMidiDevicesAppleImpl::sendMessage(const MidiMessage &m) {
	const auto b = m.getBytes();
	if(b.size()>0) {
        if(m.isSysex()) {
            sendSysex(b.data(), b.size());
        } else {
            sendBytes(b.data(), b.size());
        }
	} else {
		Log::e() << "Midi message contained no bytes!";
	}
}
