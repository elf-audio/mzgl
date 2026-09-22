#include "MidiMessageParser.h"

MidiMessageParser::MidiMessageParser(const std::function<void(const MidiData &data)> &onDataReady)
	: dataReadyCallback(onDataReady) {
	currentData.reserve(64);
	realtimeData.resize(1);
}

void MidiMessageParser::parse(const std::vector<MidiByte> &midiData,
							  uint64_t timestamp,
							  int32_t /*deviceId*/,
							  int32_t /*portId*/) {
	parse(midiData.data(), midiData.size(), timestamp);
}

void MidiMessageParser::parse(const MidiByte *midiData, size_t length, uint64_t timestamp) {
	for (size_t i = 0; i < length; i++) {
		parseByte(midiData[i], timestamp);
	}
}

void MidiMessageParser::parseByte(MidiByte byte, uint64_t timestamp) {
	const bool isStatusByte = (byte & 0x80) != 0;
	const bool inSysex		= !currentData.empty() && currentData[0] == MidiMessageConstants::MIDI_SYSEX;

	// System realtime bytes can appear anywhere, even in the middle of another
	// message. Emit them straight away without disturbing what we're building.
	if (byte >= MidiMessageConstants::MIDI_TIME_CLOCK) {
		realtimeData[0] = byte;
		dataReadyCallback({realtimeData, timestamp});
		return;
	}

	if (inSysex) {
		currentData.push_back(byte);
		if (byte == MidiMessageConstants::MIDI_SYSEX_END) {
			emitCurrent(timestamp);
		}
		return;
	}

	if (isStatusByte) {
		// a new status byte abandons any incomplete message
		currentData.clear();
		if (byte < MidiMessageConstants::MIDI_SYSEX) {
			runningStatus = byte;
		} else {
			// system common / sysex cancel running status
			runningStatus = 0;
		}
		currentData.push_back(byte);
	} else {
		if (currentData.empty()) {
			if (runningStatus == 0) {
				// stray data byte with no status to attach it to
				return;
			}
			currentData.push_back(runningStatus);
		}
		currentData.push_back(byte);
	}

	if (currentData[0] == MidiMessageConstants::MIDI_SYSEX) {
		return; // wait for 0xF7
	}

	auto expectedLength = MidiMessage::getExpectedMessageLength(currentData[0]);
	if (!expectedLength.has_value()) {
		// unknown / undefined status - drop it
		currentData.clear();
		return;
	}
	if (currentData.size() >= *expectedLength) {
		emitCurrent(timestamp);
	}
}

void MidiMessageParser::emitCurrent(uint64_t timestamp) {
	if (currentData.empty()) {
		return;
	}
	dataReadyCallback({currentData, timestamp});
	currentData.clear();
}
