#include "MidiMessageParser.h"

MidiMessageParser::MidiMessageParser(const std::function<void(const MidiData &data)> &onDataReady)
	: dataReadyCallback(onDataReady) {
}

void MidiMessageParser::parse(const std::vector<MidiByte> &midiData,
							  uint64_t timestamp,
							  int32_t deviceId,
							  int32_t portId) {
	for (auto byte: midiData) {
		currentData.push_back(byte);
		switch (currentData[0] & MIDI_SYSEX) {
			case MIDI_NOTE_OFF:
			case MIDI_NOTE_ON:
			case MIDI_PITCH_BEND:
			case MIDI_POLY_AFTERTOUCH:
			case MIDI_SONG_POS_POINTER:
			case MIDI_CONTROL_CHANGE:
				if (currentData.size() == 3) {
					emitCurrent(timestamp);
				}
				break;
			case MIDI_PROGRAM_CHANGE:
			case MIDI_AFTERTOUCH:
				if (currentData.size() == 2) {
					emitCurrent(timestamp);
				}
				break;
			case MIDI_SYSEX:
				switch (currentData.back()) {
					case MIDI_TIME_CLOCK:
					case MIDI_START:
					case MIDI_CONTINUE:
					case MIDI_STOP:
					case MIDI_ACTIVE_SENSING:
					case MIDI_SYSTEM_RESET:
					case MIDI_SYSEX_END: emitCurrent(timestamp); break;
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
	dataReadyCallback({currentData, timestamp});
	currentData.clear();
}