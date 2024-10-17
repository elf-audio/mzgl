//
//  AllMidiDevicesApple.cpp
//  mzgl
//
//  Created by Marek Bereza on 13/11/2021.
//  Copyright © 2021 Marek Bereza. All rights reserved.
//

#include "AllMidiDevicesAppleImpl.h"
#include <Foundation/Foundation.h>
#include <mach/mach_time.h>

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

	auto sendPending = [this, theDevice](MIDITimeStamp timestamp) {
		if (pendingMsg.empty()) {
			return;
		}
		midiReceived(theDevice, MidiMessage(pendingMsg), timestamp);
		pendingMsg.clear();
	};

	auto isSingleByteMessage = [](uint8_t data) { return data >= 0xF8; };
	auto isStatusByte		 = [](uint8_t data) { return data & 0x80; };

	const MIDIPacket *packet = &packetList->packet[0];
	for (int whichPacket = 0; whichPacket < packetList->numPackets; ++whichPacket) {
		for (int i = 0; i < packet->length; i++) {
			if (packet->data[i] == MidiMessageConstants::MIDI_SYSEX) {
				pendingMsg.clear();
				pendingMsg.push_back(packet->data[i]);
			} else if (packet->data[i] == MidiMessageConstants::MIDI_SYSEX_END) {
				pendingMsg.push_back(packet->data[i]);
				sendPending(packet->timeStamp);
			} else if (isSingleByteMessage(packet->data[i])) {
				pendingMsg.clear();
				pendingMsg.push_back(packet->data[i]);
				sendPending(packet->timeStamp);
			} else if (isStatusByte(packet->data[i])) {
				sendPending(packet->timeStamp);
				pendingMsg.push_back(packet->data[i]);
			} else {
				pendingMsg.push_back(packet->data[i]);
				auto expectedLength = MidiMessage::getExpectedMessageLength(pendingMsg[0]);
				if (expectedLength.has_value() && expectedLength > 0 && pendingMsg.size() >= expectedLength) {
					sendPending(packet->timeStamp);
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
										const MidiMessage &midiMessage,
										std::optional<uint64_t> timeStampInNanoSeconds) {
	const auto bytes = midiMessage.getBytes();

	mzAssert(bytes.size() < 65536);

	static constexpr auto packetExtraSpace = 100;
	Byte packetBuffer[bytes.size() + packetExtraSpace];

	auto *packetList   = (MIDIPacketList *) packetBuffer;
	MIDIPacket *packet = MIDIPacketListInit(packetList);

	uint64_t timestamp = 0;
	if (timeStampInNanoSeconds.has_value()) {
		mach_timebase_info_data_t timebaseInfo;
		mach_timebase_info(&timebaseInfo);
		uint64_t delayInHostTicks = *timeStampInNanoSeconds * timebaseInfo.denom / timebaseInfo.numer;
		timestamp			      = mach_absolute_time() + delayInHostTicks;
	}

	packet = MIDIPacketListAdd(packetList, sizeof(packetBuffer), packet, timestamp, bytes.size(), bytes.data());

	if (auto endpoint = getEndpointRefForOutputDevice(device, midiOuts); endpoint.has_value()) {
		auto result = MIDISend(outputPort, *endpoint, packetList);
		NSLogError(result, "Sending MIDI bytes");
	} else {
		NSLog(@"Failed to find end point for device");
	}
}

void AllMidiDevicesAppleImpl::removeSysexData(MIDISysexSendRequest *request) {
	sysexData.erase(std::remove_if(std::begin(sysexData),
								   std::end(sysexData),
								   [request](auto &&element) { return element.first == request; }),
					std::end(sysexData));
}

static void cleanupSysex(MIDISysexSendRequest *request) {
	if (AllMidiDevicesAppleImpl *handler = static_cast<AllMidiDevicesAppleImpl *>(request->completionRefCon)) {
		handler->removeSysexData(request);
	}
	delete request;
}

void AllMidiDevicesAppleImpl::sendSysex(const std::shared_ptr<MidiDevice> &device,
										const MidiMessage &midiMessage) {
	auto bytes = midiMessage.getBytes();

	mzAssert(bytes.size() < 65536);

	if (auto endpoint = getEndpointRefForOutputDevice(device, midiOuts); endpoint.has_value()) {
		auto request = new MIDISysexSendRequest;

		sysexData.push_back(std::pair(request, std::vector<uint8_t>(bytes.size())));

		request->data = sysexData.back().second.data();
		memcpy((void *) request->data, (void *) bytes.data(), bytes.size());

		request->bytesToSend	  = bytes.size();
		request->completionProc	  = &cleanupSysex;
		request->completionRefCon = this;
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
		sendMessage(output, midiMessage, MidiMessageConstants::NoMidiTimingApplied);
	}
}

void AllMidiDevicesAppleImpl::sendMessage(const std::shared_ptr<MidiDevice> &device,
										  const MidiMessage &midiMessage,
										  std::optional<uint64_t> timeStampInNanoSeconds) {
	if (!midiMessageHasValidBytes(midiMessage)) {
		return;
	}

	if (midiMessage.isSysex()) {
		sendSysex(device, midiMessage);
	} else {
		sendBytes(device, midiMessage, timeStampInNanoSeconds);
	}
}
