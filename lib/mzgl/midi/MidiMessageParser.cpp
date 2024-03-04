#include "MidiMessageParser.h"

MidiMessageParser::MidiMessageParser(const std::function<void(const MidiData &data)> &onDataReady)
	: dataReadyCallback(onDataReady) {
}

void MidiMessageParser::parse(const std::vector<MidiByte> &midiData,
							  uint64_t timestamp,
							  int32_t deviceId,
							  int32_t portId) {
	for (auto byte: midiData) {
		if (isStatusByte(byte)) {
			handleStatusByte(byte, timestamp);
		} else {
			handleNormalData(byte, timestamp);
		}
	}
}

bool MidiMessageParser::isStatusByte(MidiByte byte) {
	return byte & statusByte;
}

void MidiMessageParser::emitCurrent(uint64_t timestamp) {
	if (currentData.empty()) {
		return;
	}
	dataReadyCallback({currentData, timestamp});
	currentData.clear();
}

bool MidiMessageParser::currentDataIsStatusByte() const {
	return currentData.size() == 1 && currentData[0] >= 0xF6;
}

bool MidiMessageParser::hasNoCurrentDataOrOnlyAStatusByte() const {
	return currentData.empty() || (currentData[0] & 0x80) == 0;
}

void MidiMessageParser::emitCurrentIfF6OrHigher(uint64_t timestamp) {
	if (!currentDataIsStatusByte()) {
		return;
	}

	emitCurrent(timestamp);
}

void MidiMessageParser::handleStatusByte(MidiByte byte, uint64_t timestamp) {
	emitCurrent(timestamp);
	currentData.push_back(byte);
	emitCurrentIfF6OrHigher(timestamp);
}

void MidiMessageParser::handleNormalData(MidiByte byte, uint64_t timestamp) {
	if (hasNoCurrentDataOrOnlyAStatusByte()) {
		currentData.clear();
	} else {
		currentData.push_back(byte);
		switch (currentData[0] & 0xF0) {
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
			default: break;
		}
	}
}