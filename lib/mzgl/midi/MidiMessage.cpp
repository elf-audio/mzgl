#include "MidiMessage.h"
#include "mzAssert.h"

MidiMessage::MidiMessage(uint8_t byte1)
	: midiBytes {byte1, std::nullopt, std::nullopt} {
}

MidiMessage::MidiMessage(uint8_t byte1, uint8_t byte2)
	: midiBytes {byte1, byte2, std::nullopt} {
}

MidiMessage::MidiMessage(uint8_t byte1, uint8_t byte2, uint8_t byte3)
	: midiBytes {byte1, byte2, byte3} {
}

MidiMessage::MidiMessage(const uint8_t *bytes, size_t length) {
	setFromBytes(bytes, length);
}

MidiMessage::MidiMessage(const std::vector<uint8_t> &bytes) {
	setFromBytes(bytes.data(), bytes.size());
}

MidiMessage::MidiMessage(const std::initializer_list<uint8_t> &bytes) {
	setFromBytes(data(bytes), bytes.size());
}

MidiMessage::MidiMessage(const MidiMessage &other) {
	*this = other;
}

MidiMessage &MidiMessage::operator=(const MidiMessage &other) {
	if (this != &other) {
		midiBytes = other.midiBytes;
		sysexData = other.sysexData;
	}
	return *this;
}

bool MidiMessage::operator==(const MidiMessage &other) const {
	return std::tie(midiBytes, sysexData) == std::tie(other.midiBytes, other.sysexData);
}

bool MidiMessage::isNoteOn() const {
	return getMaskedStatus() == MIDI_NOTE_ON && getVelocity() > 0;
}

bool MidiMessage::isNoteOff() const {
	return getMaskedStatus() == MIDI_NOTE_OFF || (getMaskedStatus() == MIDI_NOTE_ON && getVelocity() == 0);
}
bool MidiMessage::isPitchBend() const {
	return getMaskedStatus() == MIDI_PITCH_BEND;
}

bool MidiMessage::isModWheel() const {
	return isCC() && getCC() == 1;
}

bool MidiMessage::isCC() const {
	return getMaskedStatus() == MIDI_CONTROL_CHANGE;
}

bool MidiMessage::isProgramChange() const {
	return getMaskedStatus() == MIDI_PROGRAM_CHANGE;
}

bool MidiMessage::isSysex() const {
	return getMaskedStatus() == MIDI_SYSEX;
}

bool MidiMessage::isAllNotesOff() const {
	return getMaskedStatus() == MIDI_CONTROL_CHANGE && getCC() == MIDI_CC_ALL_NOTES_OFF;
}

bool MidiMessage::isPolyPressure() const {
	return getMaskedStatus() == MIDI_POLY_AFTERTOUCH;
}

bool MidiMessage::isChannelPressure() const {
	return getMaskedStatus() == MIDI_AFTERTOUCH;
}

bool MidiMessage::isSongPositionPointer() const {
	return getMaskedStatus() == MIDI_SONG_POS_POINTER;
}

bool MidiMessage::isStart() const {
	return getMaskedStatus() == MIDI_START;
}

bool MidiMessage::isContinue() const {
	return getMaskedStatus() == MIDI_CONTINUE;
}

bool MidiMessage::isStop() const {
	return getMaskedStatus() == MIDI_STOP;
}

uint8_t MidiMessage::getChannelStatus(uint8_t message, uint8_t channel) {
	return static_cast<uint8_t>(message | std::clamp(channel - 1, 0, 15));
}

MidiMessage MidiMessage::noteOn(int channel, int pitch, int velocity) {
	return MidiMessage {getChannelStatus(MIDI_NOTE_ON, channel),
						static_cast<uint8_t>(pitch & 127),
						static_cast<uint8_t>(velocity & 127)};
}

MidiMessage MidiMessage::noteOff(int channel, int pitch) {
	return MidiMessage {getChannelStatus(MIDI_NOTE_OFF, channel), static_cast<uint8_t>(pitch & 127), 0};
}

MidiMessage MidiMessage::cc(int channel, int control, int value) {
	return MidiMessage {getChannelStatus(MIDI_CONTROL_CHANGE, channel),
						static_cast<uint8_t>(control & 127),
						static_cast<uint8_t>(value & 127)};
}

MidiMessage MidiMessage::songPositionPointer(int position) {
	return MidiMessage {MIDI_SONG_POS_POINTER,
						static_cast<uint8_t>((position & 0x3FFF) & 0x7F),
						static_cast<uint8_t>(((position & 0x3FFF) >> 7) & 0x7F)};
}

MidiMessage MidiMessage::allNotesOff() {
	return cc(0, MIDI_CC_ALL_NOTES_OFF, 0);
}

MidiMessage MidiMessage::clock() {
	return MidiMessage {MIDI_TIME_CLOCK};
}

MidiMessage MidiMessage::songStart() {
	return MidiMessage {MIDI_START};
}

MidiMessage MidiMessage::songStop() {
	return MidiMessage {MIDI_STOP};
}

uint8_t MidiMessage::getStatus() const {
	if (!midiBytes[0].has_value()) {
		mzAssert(false);
		return 0;
	}
	return *midiBytes[0];
}

uint8_t MidiMessage::getMaskedStatus() const {
	auto status = getStatus();
	return (status >= MIDI_SYSEX) ? status : status & MIDI_SYSEX;
}

uint8_t MidiMessage::getChannel() const {
	return (getStatus() & 0xF) + 1;
}

uint8_t MidiMessage::getPitch() const {
	return midiBytes[1].has_value() ? *midiBytes[1] : 0;
}

uint8_t MidiMessage::getVelocity() const {
	return midiBytes[2].has_value() ? *midiBytes[2] : 0;
}

uint8_t MidiMessage::getCC() const {
	return midiBytes[1].has_value() ? *midiBytes[1] : 0;
}

uint8_t MidiMessage::getValue() const {
	return midiBytes[2].has_value() ? *midiBytes[2] : 0;
}

std::vector<uint8_t> MidiMessage::getBytes() const {
	if (getStatus() == MIDI_SYSEX) {
		return sysexData;
	}

	std::vector<uint8_t> bytes;
	for (auto &byte: midiBytes) {
		if (byte.has_value()) {
			bytes.push_back(*byte);
		} else {
			break;
		}
	}
	return bytes;
}

float MidiMessage::getPitchBend() const {
	if (!isPitchBend()) {
		return 0.f;
	}

	auto lsb = midiBytes[1];
	auto msb = midiBytes[2];

	if (!lsb.has_value() || !msb.has_value()) {
		mzAssert(false);
		return 0.f;
	}

	auto pitchBendValue = static_cast<int>((*msb << 7) | *lsb);
	return static_cast<float>(pitchBendValue - 8192) / 8192.0f;
}

int MidiMessage::getSongPositionInMidiBeats() const {
	if (!isSongPositionPointer()) {
		return -1;
	}

	auto lsb = midiBytes[1];
	auto msb = midiBytes[2];

	if (!lsb.has_value() || !msb.has_value()) {
		mzAssert(false);
		return 0.f;
	}

	return static_cast<int>((*msb << 7) | *lsb);
}

double MidiMessage::getSongPositionInQuarterNotes() const {
	auto beats = getSongPositionInMidiBeats();
	return beats < 0 ? -1.0 : static_cast<double>(getSongPositionInMidiBeats()) / 4.0;
}

double MidiMessage::getSongPosition() const {
	return getSongPositionInQuarterNotes();
}

void MidiMessage::setFromBytes(const uint8_t *bytes, size_t length) {
	mzAssert(length > 0);
	if (length == 0) {
		mzAssert(false);
		return;
	}

	std::fill(std::begin(midiBytes), std::end(midiBytes), std::nullopt);

	if (bytes[0] == MIDI_SYSEX) {
		midiBytes[0] = MIDI_SYSEX;
		sysexData.clear();
		sysexData.insert(std::end(sysexData), bytes, bytes + length);
	} else {
		mzAssert(length <= 3);
		midiBytes[0] = bytes[0];
		if (length >= 2) {
			midiBytes[1] = bytes[1];
		}
		if (length >= 3) {
			midiBytes[2] = bytes[2];
		}
	}
}