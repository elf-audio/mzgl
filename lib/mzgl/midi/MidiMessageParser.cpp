#include "MidiMessageParser.h"

MidiMessageParser::MidiMessageParser(const std::function<void(const MidiData &data)> &onDataReady)
	: dataReadyCallback(onDataReady) {
}

void MidiMessageParser::parse(const std::vector<MidiByte> &midiData,
							  uint64_t timestamp,
							  int32_t deviceId,
							  int32_t portId) {
	for (auto byte: midiData) {
		// Handle running status: if we're starting a new message and the byte
		// is a data byte (bit 7 clear), prepend the last known status byte
		if (currentData.empty() && (byte & 0x80) == 0 && runningStatus != 0) {
			currentData.push_back(runningStatus);
		}
		currentData.push_back(byte);
		switch (currentData[0] & MidiMessageConstants::MIDI_SYSEX) {
			case MidiMessageConstants::MIDI_NOTE_OFF:
			case MidiMessageConstants::MIDI_NOTE_ON:
			case MidiMessageConstants::MIDI_PITCH_BEND:
			case MidiMessageConstants::MIDI_POLY_AFTERTOUCH:
			case MidiMessageConstants::MIDI_SONG_POS_POINTER:
			case MidiMessageConstants::MIDI_CONTROL_CHANGE:
				if (currentData.size() == 3) {
					emitCurrent(timestamp);
				}
				break;
			case MidiMessageConstants::MIDI_PROGRAM_CHANGE:
			case MidiMessageConstants::MIDI_AFTERTOUCH:
				if (currentData.size() == 2) {
					emitCurrent(timestamp);
				}
				break;
			case MidiMessageConstants::MIDI_SYSEX:
				switch (currentData.back()) {
					case MidiMessageConstants::MIDI_TIME_CLOCK:
					case MidiMessageConstants::MIDI_START:
					case MidiMessageConstants::MIDI_CONTINUE:
					case MidiMessageConstants::MIDI_STOP:
					case MidiMessageConstants::MIDI_ACTIVE_SENSING:
					case MidiMessageConstants::MIDI_SYSTEM_RESET:
					case MidiMessageConstants::MIDI_SYSEX_END: emitCurrent(timestamp); break;
					default: break;
				}
				break;
			default: break;
		}
	}
}

void MidiMessageParser::emitCurrent(uint64_t timestamp) {
	if (currentData.empty()) {
		return;
	}
	// Save running status for channel messages (0x80-0xEF)
	auto status = currentData[0];
	if (status >= 0x80 && status < 0xF0) {
		runningStatus = status;
	}
	dataReadyCallback({currentData, timestamp});
	currentData.clear();
}