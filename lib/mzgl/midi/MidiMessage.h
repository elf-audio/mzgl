//
//  MidiMessage.h
//  mzgl
//
//  Created by Marek Bereza on 10/11/2021.
//  Copyright © 2021 Marek Bereza. All rights reserved.
//

#pragma once

#include <cstdint>
#include <cstring>
#include <vector>
#include <algorithm>
#include <optional>
#include <array>

struct MidiMessageConstants {
	static constexpr auto MIDI_UNKNOWN			= 0x00;
	static constexpr auto MIDI_NOTE_OFF			= 0x80;
	static constexpr auto MIDI_NOTE_ON			= 0x90;
	static constexpr auto MIDI_CONTROL_CHANGE	= 0xB0;
	static constexpr auto MIDI_PROGRAM_CHANGE	= 0xC0;
	static constexpr auto MIDI_PITCH_BEND		= 0xE0;
	static constexpr auto MIDI_AFTERTOUCH		= 0xD0;
	static constexpr auto MIDI_POLY_AFTERTOUCH	= 0xA0;
	static constexpr auto MIDI_SYSEX			= 0xF0;
	static constexpr auto MIDI_TIME_CODE		= 0xF1;
	static constexpr auto MIDI_SONG_POS_POINTER = 0xF2;
	static constexpr auto MIDI_SONG_SELECT		= 0xF3;
	static constexpr auto MIDI_TUNE_REQUEST		= 0xF6;
	static constexpr auto MIDI_SYSEX_END		= 0xF7;
	static constexpr auto MIDI_TIME_CLOCK		= 0xF8;
	static constexpr auto MIDI_START			= 0xFA;
	static constexpr auto MIDI_CONTINUE			= 0xFB;
	static constexpr auto MIDI_STOP				= 0xFC;
	static constexpr auto MIDI_ACTIVE_SENSING	= 0xFE;
	static constexpr auto MIDI_SYSTEM_RESET		= 0xFF;
	static constexpr auto MIDI_CC_ALL_NOTES_OFF = 0x7B;
	static constexpr auto MIDI_CC_SUSTAIN_PEDAL = 0x40;

	static constexpr auto NoMidiTimingApplied = std::nullopt;
};

class MidiMessage {
public:
	MidiMessage() = default;
	explicit MidiMessage(uint8_t byte1);
	MidiMessage(uint8_t byte1, uint8_t byte2);
	MidiMessage(uint8_t byte1, uint8_t byte2, uint8_t byte3);
	MidiMessage(const uint8_t *bytes, size_t length);
	explicit MidiMessage(const std::vector<uint8_t> &bytes);
	explicit MidiMessage(const std::initializer_list<uint8_t> &bytes);
	MidiMessage(const MidiMessage &other);
	~MidiMessage() = default;

	[[maybe_unused]] MidiMessage &operator=(const MidiMessage &other);
	[[nodiscard]] bool operator==(const MidiMessage &other) const;

	[[nodiscard]] bool isNoteOn() const;
	[[nodiscard]] bool isNoteOff() const;
	[[nodiscard]] bool isPitchBend() const;
	[[nodiscard]] bool isModWheel() const;
	[[nodiscard]] bool isCC() const;
	[[nodiscard]] bool isProgramChange() const;
	[[nodiscard]] bool isSysex() const;
	[[nodiscard]] bool isAllNotesOff() const;
	[[nodiscard]] bool isPolyPressure() const;
	[[nodiscard]] bool isChannelPressure() const;
	[[nodiscard]] bool isSongPositionPointer() const;
	[[nodiscard]] bool isStart() const;
	[[nodiscard]] bool isContinue() const;
	[[nodiscard]] bool isStop() const;

	static MidiMessage noteOn(int channel, int pitch, int velocity);
	static MidiMessage noteOff(int channel, int pitch);
	static MidiMessage cc(int channel, int control, int value);

	static MidiMessage songPositionPointer(int v);
	static MidiMessage allNotesOff(int channel = 0);
	static MidiMessage clock();
	static MidiMessage songStart();
	static MidiMessage songStop();

	[[nodiscard]] uint8_t getChannel() const;
	[[nodiscard]] uint8_t getPitch() const;
	[[nodiscard]] uint8_t getVelocity() const;
	[[nodiscard]] uint8_t getCC() const;
	[[nodiscard]] uint8_t getProgramChangeIndex() const;
	[[nodiscard]] uint8_t getValue() const;
	[[nodiscard]] uint8_t getStatus() const;
	[[nodiscard]] std::vector<uint8_t> getBytes() const;
	[[nodiscard]] float getPitchBend() const;
	[[nodiscard]] int getSongPositionInMidiBeats() const;
	[[nodiscard]] double getSongPositionInQuarterNotes() const;
	[[nodiscard]] double getSongPosition() const;

	static std::optional<size_t> getExpectedMessageLength(uint8_t status);

private:
	void setFromBytes(const uint8_t *bytes, size_t length);
	[[nodiscard]] uint8_t getMaskedStatus() const;
	static uint8_t getChannelStatus(uint8_t message, uint8_t channel);

	std::array<std::optional<uint8_t>, 3> midiBytes {std::nullopt, std::nullopt, std::nullopt};
	std::vector<uint8_t> sysexData;
};
