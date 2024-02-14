//
//  MidiMessage.h
//  mzgl
//
//  Created by Marek Bereza on 10/11/2021.
//  Copyright © 2021 Marek Bereza. All rights reserved.
//

#pragma once
#include <stdint.h>
#include <string.h> // for memcpy()
#include <vector>

#define MIDI_UNKNOWN 0x00

// channel voice messages
#define MIDI_NOTE_OFF		 0x80
#define MIDI_NOTE_ON		 0x90
#define MIDI_CONTROL_CHANGE	 0xB0
#define MIDI_PROGRAM_CHANGE	 0xC0
#define MIDI_PITCH_BEND		 0xE0
#define MIDI_AFTERTOUCH		 0xD0
#define MIDI_POLY_AFTERTOUCH 0xA0

// system messages
#define MIDI_SYSEX			  0xF0 // 240
#define MIDI_TIME_CODE		  0xF1
#define MIDI_SONG_POS_POINTER 0xF2
#define MIDI_SONG_SELECT	  0xF3
#define MIDI_TUNE_REQUEST	  0xF6
#define MIDI_SYSEX_END		  0xF7
#define MIDI_TIME_CLOCK		  0xF8 // AKA midi *BEAT* clock
#define MIDI_START			  0xFA // 250 in decimal
#define MIDI_CONTINUE		  0xFB
#define MIDI_STOP			  0xFC
#define MIDI_ACTIVE_SENSING	  0xFE
#define MIDI_SYSTEM_RESET	  0xFF

// just a note for me, a CC at 123 is all notes off
#define MIDI_CC_ALL_NOTES_OFF 123
#define MIDI_CC_SUSTAIN_PEDAL 64

struct MidiMessage {
	int status	= 0;
	int channel = 0;

	// only used with RtAudio I think.
	//	double timeOffset = 0;

	union {
		int pitch;
		int control;
	};

	union {
		int velocity;
		int value;
	};

	MidiMessage() {}
	MidiMessage(int status)
		: status(status) {}
	MidiMessage(const uint8_t *bytes, int length /*, double deltaTime = 0*/) {
		setFromBytes(bytes, length);
		//		timeOffset = deltaTime;
	}

	MidiMessage(const std::vector<uint8_t> &bytes /*, double deltaTime = 0*/) {
		setFromBytes(bytes.data(), (int) bytes.size());
		//		timeOffset = deltaTime;
	}

	bool isNoteOn() const { return status == MIDI_NOTE_ON && velocity > 0; }

	bool isNoteOff() const { return status == MIDI_NOTE_OFF || (status == MIDI_NOTE_ON && velocity == 0); }

	bool isPitchBend() const { return status == MIDI_PITCH_BEND; }

	bool isModWheel() const { return status == MIDI_CONTROL_CHANGE && control == 1; }
	bool isCC() const { return status == MIDI_CONTROL_CHANGE; }
	bool isPC() const { return status == MIDI_PROGRAM_CHANGE; }
	bool isSysex() const { return status == MIDI_SYSEX; }

	bool isAllNotesOff() const { return status == MIDI_CONTROL_CHANGE && control == MIDI_CC_ALL_NOTES_OFF; }

	static MidiMessage noteOn(int channel, int pitch, int velocity) {
		MidiMessage m;
		m.status   = MIDI_NOTE_ON;
		m.channel  = channel;
		m.velocity = velocity;
		m.pitch	   = pitch;
		return m;
	}

	static MidiMessage noteOff(int channel, int pitch) {
		MidiMessage m;
		m.status   = MIDI_NOTE_OFF;
		m.channel  = channel;
		m.velocity = 0;
		m.pitch	   = pitch;
		return m;
	}

	static MidiMessage cc(int channel, int control, int value) {
		MidiMessage m;
		m.status  = MIDI_CONTROL_CHANGE;
		m.channel = channel;
		m.control = control;
		m.value	  = value;
		return m;
	}

	static MidiMessage songPositionPointer(int v) {
		MidiMessage m(MIDI_SONG_POS_POINTER);
		m.value = v;
		return m;
	}
	static MidiMessage allNotesOff() { return cc(0, MIDI_CC_ALL_NOTES_OFF, 0); }
	static MidiMessage clock() { return MidiMessage(MIDI_TIME_CLOCK); }
	static MidiMessage songStart() { return MidiMessage(MIDI_START); }
	static MidiMessage songStop() { return MidiMessage(MIDI_STOP); }

	std::vector<uint8_t> getBytes() const {
		if (status == MIDI_SYSEX) {
			return sysexData;
		}
		std::vector<uint8_t> bytes;

		bytes.push_back(status + (channel - 1));
		switch (status) {
			case MIDI_NOTE_ON:
			case MIDI_NOTE_OFF:
				bytes.push_back(pitch);
				bytes.push_back(velocity);
				break;
			case MIDI_CONTROL_CHANGE:
				bytes.push_back(control);
				bytes.push_back(value);
				break;
			case MIDI_PROGRAM_CHANGE:
			case MIDI_AFTERTOUCH: bytes.push_back(value); break;
			case MIDI_PITCH_BEND:
				bytes.push_back(value & 0x7F); // lsb 7bit
				bytes.push_back((value >> 7) & 0x7F); // msb 7bit
				break;
			case MIDI_POLY_AFTERTOUCH:
				bytes.push_back(pitch);
				bytes.push_back(value);
				break;
			case MIDI_SONG_POS_POINTER:
				bytes.push_back(value & 0x7F); // lsb 7bit
				bytes.push_back((value >> 7) & 0x7F); // msb 7bit
				break;

			default:
				//printf("Unknown message type : %d\n", status);
				break;
		}
		return bytes;
	}

	float getPitchBend() const {
		if (value > 8192) {
			return (value - 8191) / 8192.f;
		} else {
			return (value - 8192) / 8192.f;
		}
	}

private:
	std::vector<uint8_t> sysexData;
	void setFromBytes(const uint8_t *bytes, int length) {
		if (bytes[0] > MIDI_SYSEX) {
			status	= (bytes[0] & 0xFF);
			channel = 0;
		} else if (bytes[0] == MIDI_SYSEX) {
			status	= MIDI_SYSEX;
			channel = 0;
			sysexData.resize(length);
			memcpy(sysexData.data(), bytes, length);

		} else {
			status	= (bytes[0] & 0xF0);
			channel = (int) (bytes[0] & 0x0F) + 1;
		}

		switch (status) {
			case MIDI_NOTE_ON:
			case MIDI_NOTE_OFF:
				pitch	 = (int) bytes[1];
				velocity = (int) bytes[2];

				break;
			case MIDI_CONTROL_CHANGE:
				control = (int) bytes[1];
				value	= (int) bytes[2];
				break;
			case MIDI_PROGRAM_CHANGE:
			case MIDI_AFTERTOUCH: value = (int) bytes[1]; break;
			case MIDI_PITCH_BEND:
				value = (int) (bytes[2] << 7) + (int) bytes[1]; // msb + lsb
				break;
			case MIDI_POLY_AFTERTOUCH:
				pitch = (int) bytes[1];
				value = (int) bytes[2];
				break;
			case MIDI_SONG_POS_POINTER:
				value = (int) (bytes[2] << 7) + (int) bytes[1]; // msb + lsb
				break;
			default:
				//printf("Unknown message type : %d\n", status);
				break;
		}
	}
};
