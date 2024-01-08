//
// Created by Marek Bereza on 04/01/2024.
//

#include "midiMessagePrinting.h"
#include <iomanip>

std::string bytesToHex(const std::vector<uint8_t> &bytes) {
	std::stringstream ss;
	ss << std::hex << std::setfill('0');
	for (uint8_t byte: bytes) {
		ss << std::setw(2) << static_cast<int>(byte) << " ";
	}
	return ss.str();
}
std::ostream &operator<<(std::ostream &os, const MidiMessage &midiMsg) {
	std::vector<uint8_t> msg = midiMsg.getBytes();
	os << "[MIDI ";

	if (msg.empty()) {
		os << "(empty)]";
		return os;
	}

	uint8_t statusByte	= msg[0];
	uint8_t messageType = statusByte & 0xF0;
	uint8_t channel		= statusByte & 0x0F;

	switch (messageType) {
		case MIDI_NOTE_OFF: os << "note off, ch: " << +channel; break;
		case MIDI_NOTE_ON: os << "note on, ch: " << +channel; break;
		case MIDI_POLY_AFTERTOUCH: os << "Polyphonic Key Pressure (Aftertouch), ch: " << +channel; break;
		case MIDI_CONTROL_CHANGE: os << "CC, ch: " << +channel; break;
		case MIDI_PROGRAM_CHANGE: os << "PC, ch: " << +channel; break;
		case MIDI_AFTERTOUCH: os << "Channel Pressure (Aftertouch), ch: " << +channel; break;
		case MIDI_PITCH_BEND: os << "pitchbend, ch: " << +channel; break;
		case MIDI_SYSEX:
			os << "SysEx: " << bytesToHex(msg) << "]";
			return os; // Early return for SysEx messages
		// Additional System Messages
		case MIDI_TIME_CODE:
		case MIDI_SONG_POS_POINTER:
		case MIDI_SONG_SELECT:
		case MIDI_TUNE_REQUEST:
		case MIDI_SYSEX_END:
		case MIDI_TIME_CLOCK:
		case MIDI_START:
		case MIDI_CONTINUE:
		case MIDI_STOP:
		case MIDI_ACTIVE_SENSING:
		case MIDI_SYSTEM_RESET:
			// You can add specific handling for each of these cases if needed
			os << "System Message: 0x" << std::hex << static_cast<int>(statusByte);
			break;
		default: os << "Unknown Message"; break;
	}

	if (msg.size() > 1) {
		os << ", Data1: " << +msg[1];
	}
	if (msg.size() > 2) {
		os << ", Data2: " << +msg[2];
	}

	os << "]";
	return os;
}