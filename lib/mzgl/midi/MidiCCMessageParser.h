#pragma once

#include "MidiCCModeDetector.h"

class MidiCCMessageParser {
public:
	enum class ChangeType { NotApplicable, Positive, Negative, NoChange };

	[[nodiscard]] ChangeType determineChange(const MidiMessage &message, MidiCCModeDetector::MidiCCMode mode);

	static std::string toString(ChangeType mode);
	static int toValue(ChangeType mode);
	static ChangeType toType(const std::string &str);

private:
	ChangeType decodeTwosComplement(const MidiMessage &message);
	ChangeType decodeMinMax1(const MidiMessage &message);
	ChangeType decodeMinMax2(const MidiMessage &message);
	ChangeType decodeSignedBit1(const MidiMessage &message);
	ChangeType decodeSignedBit2(const MidiMessage &message);
	ChangeType decodeRelativeBinaryOffset(const MidiMessage &message);
	ChangeType decodeArturiaRelative1(const MidiMessage &message);
	ChangeType decodeArturiaRelative2(const MidiMessage &message);
	ChangeType decodeArturiaRelative3(const MidiMessage &message);
};
