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
#include "mainThread.h"
#include "util.h"
#include <memory.h>
#include "mzAssert.h"

static void NSLogError(OSStatus c, const std::string &str) {
	if (c != noErr) {
		printf("Error trying to : %s, with status %d\n", str.c_str(), c);
	}
}

static std::string nameOfEndpoint(MIDIEndpointRef ref) {
	CFStringRef string = nil;
	OSStatus s		   = MIDIObjectGetStringProperty(ref, kMIDIPropertyDisplayName, (CFStringRef *) &string);
	if (s != noErr) {
		return "Unknown name";
	}
	auto str = (NSString *) CFBridgingRelease(string);
	return [str UTF8String];
}

CoreMidiDevice::CoreMidiDevice(MIDIEndpointRef endpoint)
	: endpoint(endpoint) {
	name = nameOfEndpoint(endpoint);
}

std::shared_ptr<MidiDevice> getInputDeviceForEndpointRef(const CoreMidiDevice &device,
														 const std::vector<CoreMidiInRef> &midiIns) {
	for (const auto &src: midiIns) {
		if (src->endpoint == device.endpoint) {
			return src;
		}
	}
	return nullptr;
}

std::optional<MIDIEndpointRef> getEndpointRefForOutputDevice(const std::shared_ptr<MidiDevice> &device,
															 const std::vector<CoreMidiOutRef> &midiOuts) {
	for (ItemCount index = 0; index < MIDIGetNumberOfDestinations(); ++index) {
		auto endpoint = MIDIGetDestination(index);
		if (endpoint == 0) {
			continue;
		}

		for (const auto &destination: midiOuts) {
			if (destination->endpoint == endpoint && *device == *destination) {
				return endpoint;
			}
		}
	}

	return std::nullopt;
}

static void MIDIReadNoteProc(const MIDIPacketList *packetList, void *readProcRefCon, void *srcConnRefCon) {
	auto *me  = (AllMidiDevicesAppleImpl *) readProcRefCon;
	auto *dev = (CoreMidiDevice *) srcConnRefCon;
	me->packetListReceived(*dev, packetList);
}

static void myMIDINotifyProc(const MIDINotification *message, void *refCon) {
	auto *me = (AllMidiDevicesAppleImpl *) refCon;
	me->midiNotify(message);
}

static void removeFirst(std::vector<CoreMidiInRef> &devs, const CoreMidiInRef &dev) {
	for (int i = 0; i < devs.size(); i++) {
		if (devs[i] == dev) {
			devs.erase(devs.begin() + i);
			return;
		}
	}
}
static void removeFirst(std::vector<CoreMidiOutRef> &devs, const CoreMidiOutRef &dev) {
	for (int i = 0; i < devs.size(); i++) {
		if (devs[i] == dev) {
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
	if (running) return;
	// keep checking for new ports
	running			  = true;
	portScannerThread = std::thread([this]() {
#ifdef DEBUG
		setThreadName("AllMidiIns::portScanner");
#endif
		std::atomic<bool> doneScanning = true;
		auto *ptr					   = &doneScanning;
		while (running) {
			[[NSOperationQueue mainQueue] addOperationWithBlock:^{
			  if (running) scanForDevices();
			}];

			for (int i = 0; i < 100; i++) {
				sleepMillis(10);
				if (!running) break;
			}
		}
	});
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
	running = false;
	[[NSOperationQueue mainQueue] cancelAllOperations];
	try {
		if (portScannerThread.joinable()) {
			portScannerThread.join();
		}
	} catch (std::system_error &e) {
		Log::d() << "Got error trying to join portScannerThread - maybe it's not running? - error: " << e.what();
	}
}

void AllMidiDevicesAppleImpl::scanForDevices() {
	const ItemCount numberOfDestinations = MIDIGetNumberOfDestinations();
	const ItemCount numberOfSources		 = MIDIGetNumberOfSources();

	auto removedSources		 = midiIns;
	auto removedDestinations = midiOuts;

	for (ItemCount index = 0; index < numberOfDestinations; ++index) {
		MIDIEndpointRef endpoint = MIDIGetDestination(index);
		if (endpoint == virtualDestinationEndpoint) continue;

		bool matched = false;
		for (const auto &destination: midiOuts) {
			if (destination->endpoint == endpoint) {
				removeFirst(removedDestinations, destination);
				matched = true;
				break;
			}
		}
		if (matched) continue;
		connectOutput(endpoint);
	}

	for (ItemCount index = 0; index < numberOfSources; ++index) {
		MIDIEndpointRef endpoint = MIDIGetSource(index);
		if (endpoint == virtualSourceEndpoint) continue;

		bool matched = false;
		for (const auto &source: midiIns) {
			if (source->endpoint == endpoint) {
				removeFirst(removedSources, source);
				matched = true;
				break;
			}
		}
		if (matched) continue;
		connectInput(endpoint);
	}

	for (const auto &destination: removedDestinations) {
		disconnectOutput(destination->endpoint);
	}

	for (const auto &source: removedSources) {
		disconnectInput(source->endpoint);
	}
}

void AllMidiDevicesAppleImpl::connectOutput(MIDIEndpointRef endpoint) {
	//    Log::d() << "Connecting destination " << nameOfEndpoint(endpoint);
	auto dest = CoreMidiOut::create(endpoint);
	midiOuts.push_back(dest);
	notifyConnection(dest);
	//	notifyConnectionChange();
}

void AllMidiDevicesAppleImpl::disconnectOutput(MIDIEndpointRef endpoint) {
	Log::d() << "Disconnecting destination " << nameOfEndpoint(endpoint);
	auto dest = getOutput(endpoint);
	if (dest) {
		removeFirst(midiOuts, dest);
		notifyDisconnection(dest);
	}
}

void AllMidiDevicesAppleImpl::connectInput(MIDIEndpointRef endpoint) {
	Log::d() << "Connecting source " << nameOfEndpoint(endpoint);
	auto src = CoreMidiIn::create(endpoint);
	midiIns.push_back(src);

	notifyConnection(src);
	OSStatus s = MIDIPortConnectSource(inputPort, endpoint, (void *) src.get());
	NSLogError(s, "Connecting to MIDI source");
}

CoreMidiInRef AllMidiDevicesAppleImpl::getInput(MIDIEndpointRef endpoint) {
	for (auto s: midiIns) {
		if (s->endpoint == endpoint) return s;
	}
	return nullptr;
}

CoreMidiOutRef AllMidiDevicesAppleImpl::getOutput(MIDIEndpointRef endpoint) {
	for (auto d: midiOuts) {
		if (d->endpoint == endpoint) return d;
	}
	return nullptr;
}

void AllMidiDevicesAppleImpl::disconnectInput(MIDIEndpointRef endpoint) {
	Log::d() << "Disconnecting source " << nameOfEndpoint(endpoint);
	auto src = getInput(endpoint);
	if (src) {
		OSStatus s = MIDIPortDisconnectSource(inputPort, endpoint);
		NSLogError(s, "Disconnecting from MIDI source");
		// [sources removeObject:source];
		removeFirst(midiIns, src);
		//		notifyConnectionChange();
		notifyDisconnection(src);
	}
}

void AllMidiDevicesAppleImpl::midiReceived(const std::shared_ptr<MidiDevice> &device,
										   const MidiMessage &msg,
										   uint64_t timestamp) {
	uint64_t ts = hostTicksToNanoSeconds(timestamp);
	for (auto *listener: listeners) {
		listener->midiReceived(device, msg, ts);
	}
}

void AllMidiDevicesAppleImpl::packetListReceived(const CoreMidiDevice &device, const MIDIPacketList *packetList) {
	// there is a bug here probably if you were to send multiple streams of data
	// there is no distinguishing between different devices in regard to pendingMsg.
	// you'd need one per device.
	// Log::e() << "Packet list received from " << device.name;
	auto theDevice = getInputDeviceForEndpointRef(device, midiIns);
	if (theDevice == nullptr) {
		return;
	}

	const MIDIPacket *packet = &packetList->packet[0];
	for (int whichPacket = 0; whichPacket < packetList->numPackets; ++whichPacket) {
		for (int i = 0; i < packet->length; i++) {
			auto *d = packet->data;

			if (d[i] & 0x80) {
				// this is a status byte, send any previous messages
				if (!pendingMsg.empty()) {
					midiReceived(theDevice, MidiMessage(pendingMsg), packet->timeStamp);
					pendingMsg.clear();
				}

				pendingMsg.push_back(d[i]);

				// if this status byte is for a 1 byte message send it straight away
				// all status 0xF6 and above are all the 1 byte messages.
				if (pendingMsg.size() == 1 && pendingMsg[0] >= 0xF6) {
					midiReceived(theDevice, MidiMessage(pendingMsg), packet->timeStamp);
					pendingMsg.clear();
				}
			} else {
				// a data byte

				// first check we have a status in the gun, otherwise can this message
				if (pendingMsg.empty() || (pendingMsg[0] & 0x80) == 0) {
					pendingMsg.clear();
				} else {
					pendingMsg.push_back(d[i]);
					// now check to see if the message is finished
					// by seeing which status we have and looking it
					// up in known statuses/lengths
					int status		= pendingMsg[0] & 0xF0;
					bool shouldEmit = false;
					switch (status) {
						case MIDI_NOTE_OFF:
						case MIDI_NOTE_ON:
						case MIDI_PITCH_BEND:
						case MIDI_POLY_AFTERTOUCH:
						case MIDI_SONG_POS_POINTER:
						case MIDI_CONTROL_CHANGE:
							if (pendingMsg.size() == 3) shouldEmit = true;
							break;
						case MIDI_PROGRAM_CHANGE:
						case MIDI_AFTERTOUCH:
						case MIDI_SONG_SELECT:
							if (pendingMsg.size() == 2) shouldEmit = true;
							break;
						default: // status unknown, don't send, next message will push this through
							break;
					}
					if (shouldEmit) {
						midiReceived(theDevice, MidiMessage(pendingMsg), packet->timeStamp);
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

	if (notification->childType == kMIDIObjectType_Destination)
		connectOutput((MIDIEndpointRef) notification->child);
	else if (notification->childType == kMIDIObjectType_Source)
		connectInput((MIDIEndpointRef) notification->child);
}

void AllMidiDevicesAppleImpl::midiNotifyRemove(const MIDIObjectAddRemoveNotification *notification) {
	if (notification->child == virtualDestinationEndpoint || notification->child == virtualSourceEndpoint) return;

	if (notification->childType == kMIDIObjectType_Destination)
		disconnectOutput((MIDIEndpointRef) notification->child);
	else if (notification->childType == kMIDIObjectType_Source)
		disconnectInput((MIDIEndpointRef) notification->child);
}

void AllMidiDevicesAppleImpl::midiNotify(const MIDINotification *notification) {
	switch (notification->messageID) {
		case kMIDIMsgObjectAdded: midiNotifyAdd((const MIDIObjectAddRemoveNotification *) notification); break;
		case kMIDIMsgObjectRemoved:
			midiNotifyRemove((const MIDIObjectAddRemoveNotification *) notification);
			break;
		case kMIDIMsgSetupChanged:
		case kMIDIMsgPropertyChanged:
		case kMIDIMsgThruConnectionsChanged:
		case kMIDIMsgSerialPortOwnerChanged:
		case kMIDIMsgIOError: break;
	}
}

std::vector<std::shared_ptr<MidiDevice>> AllMidiDevicesAppleImpl::getConnectedMidiDevices() const {
	std::vector<std::shared_ptr<MidiDevice>> devices;
	devices.insert(std::end(devices), std::begin(midiIns), std::end(midiIns));
	devices.insert(std::end(devices), std::begin(midiOuts), std::end(midiOuts));
	return devices;
}

void AllMidiDevicesAppleImpl::sendBytes(const std::shared_ptr<MidiDevice> &device,
										const MidiMessage &midiMessage) {
	const auto bytes = midiMessage.getBytes();

	mzAssert(bytes.size() < 65536);

	static constexpr auto packetExtraSpace = 100;
	Byte packetBuffer[bytes.size() + packetExtraSpace];

	auto *packetList   = (MIDIPacketList *) packetBuffer;
	MIDIPacket *packet = MIDIPacketListInit(packetList);

	packet = MIDIPacketListAdd(packetList, sizeof(packetBuffer), packet, 0, bytes.size(), bytes.data());

	if (auto endpoint = getEndpointRefForOutputDevice(device, midiOuts); endpoint.has_value()) {
		auto result = MIDISend(outputPort, *endpoint, packetList);
		NSLogError(result, "Sending MIDI bytes");
	} else {
		NSLog(@"Failed to find end point for device");
	}
}

static void cleanupSysex(MIDISysexSendRequest *request) {
	delete request;
}

void AllMidiDevicesAppleImpl::sendSysex(const std::shared_ptr<MidiDevice> &device,
										const MidiMessage &midiMessage) {
	auto bytes = midiMessage.getBytes();

	assert(bytes.size() < 65536);

	if (auto endpoint = getEndpointRefForOutputDevice(device, midiOuts); endpoint.has_value()) {
		auto request	= new MIDISysexSendRequest;
		const auto size = midiMessage.getBytes().size();

		request->data = new Byte[bytes.size()];
		memcpy((void *) request->data, (void *) bytes.data(), bytes.size());

		request->bytesToSend	  = size;
		request->completionProc	  = &cleanupSysex;
		request->completionRefCon = request;
		request->complete		  = false;
		request->destination	  = *endpoint;

		auto result = MIDISendSysex(request);
		NSLogError(result, "Sending MIDI sysex");
	}
}

bool midiMessageHasValidBytes(const MidiMessage &midiMessage) {
	if (midiMessage.getBytes().empty()) {
		Log::e() << "Midi message contained no bytes!";
		return false;
	}

	return true;
}

void AllMidiDevicesAppleImpl::sendMessage(const MidiMessage &midiMessage) {
	if (!midiMessageHasValidBytes(midiMessage)) {
		return;
	}

	for (const auto &output: midiOuts) {
		sendMessage(output, midiMessage);
	}
}

void AllMidiDevicesAppleImpl::sendMessage(const std::shared_ptr<MidiDevice> &device,
										  const MidiMessage &midiMessage) {
	if (!midiMessageHasValidBytes(midiMessage)) {
		return;
	}

	if (midiMessage.isSysex()) {
		sendSysex(device, midiMessage);
	} else {
		sendBytes(device, midiMessage);
	}
}
