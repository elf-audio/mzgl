#include "MidiCCMessageParser.h"
#include "mzAssert.h"

MidiCCMessageParser::ChangeType MidiCCMessageParser::determineChange(const MidiMessage &message,
																	 MidiCCModeDetector::MidiCCMode mode) {
	switch (mode) {
		case MidiCCModeDetector::MidiCCMode::Unknown:
		case MidiCCModeDetector::MidiCCMode::Absolute: return MidiCCMessageParser::ChangeType::NotApplicable;
		case MidiCCModeDetector::MidiCCMode::TwosComplement: return decodeTwosComplement(message);
		case MidiCCModeDetector::MidiCCMode::MinMax1: return decodeMinMax1(message);
		case MidiCCModeDetector::MidiCCMode::MinMax2: return decodeMinMax2(message);
		case MidiCCModeDetector::MidiCCMode::SignedBit1: return decodeSignedBit1(message);
		case MidiCCModeDetector::MidiCCMode::SignedBit2: return decodeSignedBit2(message);
		case MidiCCModeDetector::MidiCCMode::RelativeBinaryOffset: return decodeRelativeBinaryOffset(message);
		case MidiCCModeDetector::MidiCCMode::ArturiaRelative1: return decodeArturiaRelative1(message);
		case MidiCCModeDetector::MidiCCMode::ArturiaRelative2: return decodeArturiaRelative2(message);
		case MidiCCModeDetector::MidiCCMode::ArturiaRelative3: return decodeArturiaRelative3(message);
	}
	mzAssert(false);
	return MidiCCMessageParser::ChangeType::NotApplicable;
}

std::vector<std::pair<MidiCCMessageParser::ChangeType, std::string>> getMidiCCChangeType() {
	return {{MidiCCMessageParser::ChangeType::NotApplicable, "NotApplicable"},
			{MidiCCMessageParser::ChangeType::Positive, "Positive"},
			{MidiCCMessageParser::ChangeType::Negative, "Negative"},
			{MidiCCMessageParser::ChangeType::NoChange, "NoChange"}};
}

std::string MidiCCMessageParser::toString(MidiCCMessageParser::ChangeType type) {
	auto names = getMidiCCChangeType();
	auto iter  = std::find_if(names.begin(), names.end(), [type](auto &&pair) { return type == pair.first; });
	if (iter == names.end()) {
		throw std::invalid_argument("Invalid ChangeType");
	}
	return iter->second;
}

int MidiCCMessageParser::toValue(MidiCCMessageParser::ChangeType type) {
	switch (type) {
		case MidiCCMessageParser::ChangeType::NotApplicable: return 0;
		case MidiCCMessageParser::ChangeType::Positive: return 1;
		case MidiCCMessageParser::ChangeType::Negative: return -1;
		case MidiCCMessageParser::ChangeType::NoChange: return 0;
	}
	return 0;
}

MidiCCMessageParser::ChangeType MidiCCMessageParser::toType(const std::string &type) {
	auto names = getMidiCCChangeType();
	auto iter  = std::find_if(names.begin(), names.end(), [type](auto &&pair) { return type == pair.second; });
	if (iter == names.end()) {
		throw std::invalid_argument("Invalid MidiCCMode string");
	}
	return iter->first;
}

MidiCCMessageParser::ChangeType MidiCCMessageParser::decodeTwosComplement(const MidiMessage &message) {
	auto dataByte = static_cast<int>(message.getValue());
	if (dataByte < 0 || dataByte > 127) {
		return MidiCCMessageParser::ChangeType::NotApplicable;
	}

	auto delta = dataByte;
	if (dataByte > 63) {
		delta = dataByte - 128;
	}

	if (delta > 0) {
		return MidiCCMessageParser::ChangeType::Positive;
	}
	if (delta < 0) {
		return MidiCCMessageParser::ChangeType::Negative;
	}
	return ChangeType::NoChange;
}

MidiCCMessageParser::ChangeType MidiCCMessageParser::decodeMinMax1(const MidiMessage &message) {
	auto [minValue, maxValue] = MidiCCModeDetector::getMinMax1Values();
	if (message.getValue() == maxValue) {
		return ChangeType::Negative;
	}
	if (message.getValue() == minValue) {
		return ChangeType::Positive;
	}
	return ChangeType::NoChange;
}

MidiCCMessageParser::ChangeType MidiCCMessageParser::decodeMinMax2(const MidiMessage &message) {
	auto [minValue, maxValue] = MidiCCModeDetector::getMinMax2Values();
	if (message.getValue() == maxValue) {
		return ChangeType::Negative;
	}
	if (message.getValue() == minValue) {
		return ChangeType::Positive;
	}
	return ChangeType::NoChange;
}

MidiCCMessageParser::ChangeType MidiCCMessageParser::decodeSignedBit1(const MidiMessage &message) {
	auto dataByte = static_cast<int>(message.getValue());
	if (dataByte < 0 || dataByte > 127) {
		return MidiCCMessageParser::ChangeType::NotApplicable;
	}

	int sign	  = (dataByte & 0x40) ? -1 : 1;
	int magnitude = dataByte & 0x3F;

	if (magnitude == 0) {
		return MidiCCMessageParser::ChangeType::NoChange;
	}

	return (sign * magnitude > 0) ? MidiCCMessageParser::ChangeType::Positive
								  : MidiCCMessageParser::ChangeType::Negative;
}

MidiCCMessageParser::ChangeType MidiCCMessageParser::decodeSignedBit2(const MidiMessage &message) {
	auto dataByte = static_cast<int>(message.getValue());
	if (dataByte < 0 || dataByte > 127) {
		return MidiCCMessageParser::ChangeType::NotApplicable;
	}

	int sign	  = (dataByte & 0x40) ? 1 : -1;
	int magnitude = dataByte & 0x3F;

	if (magnitude == 0) {
		return MidiCCMessageParser::ChangeType::NoChange;
	}

	return (sign * magnitude > 0) ? MidiCCMessageParser::ChangeType::Positive
								  : MidiCCMessageParser::ChangeType::Negative;
}

MidiCCMessageParser::ChangeType MidiCCMessageParser::decodeRelativeBinaryOffset(const MidiMessage &message) {
	auto [minValue, maxValue] = MidiCCModeDetector::getRelativeBinaryRange();
	if (message.getValue() == maxValue) {
		return ChangeType::Negative;
	}
	if (message.getValue() == minValue) {
		return ChangeType::Positive;
	}
	if (message.getValue() == ((maxValue + minValue) / 2)) {
		return ChangeType::NoChange;
	}
	return ChangeType::NoChange;
}

MidiCCMessageParser::ChangeType MidiCCMessageParser::decodeArturiaRelative1(const MidiMessage &message) {
	if (message.getValue() == 0) {
		return ChangeType::NoChange;
	}

	return (message.getValue() < 64) ? ChangeType::Negative : ChangeType::Positive;
}

MidiCCMessageParser::ChangeType MidiCCMessageParser::decodeArturiaRelative2(const MidiMessage &message) {
	if (message.getValue() == 0) {
		return ChangeType::NoChange;
	}

	return (message.getValue() <= 3) ? ChangeType::Positive : ChangeType::Negative;
}

MidiCCMessageParser::ChangeType MidiCCMessageParser::decodeArturiaRelative3(const MidiMessage &message) {
	if (message.getValue() == 0) {
		return ChangeType::NoChange;
	}
	return (message.getValue() <= 15) ? ChangeType::Negative : ChangeType::Positive;
}
