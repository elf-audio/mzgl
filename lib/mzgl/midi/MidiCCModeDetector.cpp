#include "MidiCCModeDetector.h"

MidiCCModeDetector::MidiCCModeDetector() {
	for (auto &ccInfo: ccInfos) {
		ccInfo = CCInfo {};
	}
}

void MidiCCModeDetector::process(const MidiMessage &message) {
	if (!message.isCC()) {
		return;
	}

	auto ccIndex = message.getCC();
	auto value	 = message.getValue();

	if (ccIndex >= numberOfCCs) {
		throw std::out_of_range("Invalid CC index");
	}

	ccInfos[ccIndex].buffer.push(value);
	ccInfos[ccIndex].mode = detectCCMode(ccInfos[ccIndex].buffer);
}

MidiCCModeDetector::MidiCCMode MidiCCModeDetector::getDetectedMode(uint8_t ccIndex) const {
	if (ccIndex >= numberOfCCs) {
		throw std::out_of_range("Invalid CC index");
	}
	return ccInfos[ccIndex].mode;
}

std::vector<std::pair<MidiCCModeDetector::MidiCCMode, std::string>> getMidiCCModeNameMap() {
	return {
		{MidiCCModeDetector::MidiCCMode::Absolute, "Absolute"},
		{MidiCCModeDetector::MidiCCMode::TwosComplement, "TwosComplement"},
		{MidiCCModeDetector::MidiCCMode::MinMax1, "MinMax1"},
		{MidiCCModeDetector::MidiCCMode::MinMax2, "MinMax2"},
		{MidiCCModeDetector::MidiCCMode::RelativeBinaryOffset, "RelativeBinaryOffset"},
		{MidiCCModeDetector::MidiCCMode::SignedBit1, "SignedBit1"},
		{MidiCCModeDetector::MidiCCMode::SignedBit2, "SignedBit2"},
		{MidiCCModeDetector::MidiCCMode::ArturiaRelative1, "ArturiaRelative1"},
		{MidiCCModeDetector::MidiCCMode::ArturiaRelative2, "ArturiaRelative2"},
		{MidiCCModeDetector::MidiCCMode::ArturiaRelative3, "ArturiaRelative3"},
		{MidiCCModeDetector::MidiCCMode::Unknown, "Unknown"},
	};
}

std::string MidiCCModeDetector::toString(MidiCCMode mode) {
	auto names = getMidiCCModeNameMap();
	auto iter  = std::find_if(names.begin(), names.end(), [mode](auto &&pair) { return mode == pair.first; });
	if (iter == names.end()) {
		throw std::invalid_argument("Invalid MidiCCMode");
	}
	return iter->second;
}

MidiCCModeDetector::MidiCCMode MidiCCModeDetector::toMode(const std::string &mode) {
	auto names = getMidiCCModeNameMap();
	auto iter  = std::find_if(names.begin(), names.end(), [mode](auto &&pair) { return mode == pair.second; });
	if (iter == names.end()) {
		throw std::invalid_argument("Invalid MidiCCMode string");
	}
	return iter->first;
}

std::pair<int, int> MidiCCModeDetector::getMinMax1Values() {
	static constexpr auto minValue = 1;
	static constexpr auto maxValue = 127;
	return {minValue, maxValue};
}

std::pair<int, int> MidiCCModeDetector::getMinMax2Values() {
	static constexpr auto minValue = 63;
	static constexpr auto maxValue = 65;
	return {minValue, maxValue};
}

std::pair<int, int> MidiCCModeDetector::getRelativeBinaryRange() {
	static constexpr auto minValue = 63;
	static constexpr auto maxValue = 65;
	return {minValue, maxValue};
}

std::pair<int, int> MidiCCModeDetector::getArturiaRelative1Range() {
	static constexpr auto minValue = 61;
	static constexpr auto maxValue = 67;
	return {minValue, maxValue};
}

bool MidiCCModeDetector::allMessagesInRange(const FixedSizeCircularBuffer<uint8_t, maxMessages> &messages,
											int min,
											int max,
											MidiCCModeDetector::CanBeZero canBeZero) {
	bool foundZero = false;
	for (auto value: messages) {
		if ((canBeZero == CanBeZero::Yes || canBeZero == CanBeZero::Must) && value == 0) {
			foundZero = true;
			continue;
		}
		if (value >= min && value <= max) {
			continue;
		}
		return false;
	}

	if (canBeZero == CanBeZero::Must) {
		return foundZero;
	}

	return true;
}

bool MidiCCModeDetector::allMessagesInRange(const FixedSizeCircularBuffer<uint8_t, maxMessages> &messages,
											int range1Min,
											int range1Max,
											int range2Min,
											int range2Max,
											MidiCCModeDetector::CanBeZero canBeZero) {
	bool foundZero = false;
	for (auto value: messages) {
		if ((canBeZero == CanBeZero::Yes || canBeZero == CanBeZero::Must) && value == 0) {
			foundZero = true;
			continue;
		}
		if (value >= range1Min && value <= range1Max) {
			continue;
		}
		if (value >= range2Min && value <= range2Max) {
			continue;
		}
		return false;
	}

	if (canBeZero == CanBeZero::Must) {
		return foundZero;
	}

	return true;
}

bool MidiCCModeDetector::allMessagesExactly(const FixedSizeCircularBuffer<uint8_t, maxMessages> &messages,
											int min,
											int max) {
	for (auto value: messages) {
		if (value != min && value != max) {
			return false;
		}
	}
	return true;
}

bool MidiCCModeDetector::isArturiaRelative1(const FixedSizeCircularBuffer<uint8_t, maxMessages> &messages) {
	auto [minValue, maxValue] = getArturiaRelative1Range();
	return allMessagesInRange(messages, minValue, maxValue, CanBeZero::Must);
}

bool MidiCCModeDetector::isArturiaRelative2(const FixedSizeCircularBuffer<uint8_t, maxMessages> &messages) {
	static constexpr auto range1MinValue = 1;
	static constexpr auto range1MaxValue = 3;
	static constexpr auto range2MinValue = 125;
	static constexpr auto range2MaxValue = 127;
	return allMessagesInRange(
		messages, range1MinValue, range1MaxValue, range2MinValue, range2MaxValue, CanBeZero::Must);
}

bool MidiCCModeDetector::isArturiaRelative3(const FixedSizeCircularBuffer<uint8_t, maxMessages> &messages) {
	static constexpr auto range1MinValue = 12;
	static constexpr auto range1MaxValue = 15;
	static constexpr auto range2MinValue = 17;
	static constexpr auto range2MaxValue = 19;
	return allMessagesInRange(
		messages, range1MinValue, range1MaxValue, range2MinValue, range2MaxValue, CanBeZero::Must);
}

bool MidiCCModeDetector::isRelativeBinaryOffset(const FixedSizeCircularBuffer<uint8_t, maxMessages> &messages) {
	auto [minValue, maxValue] = getRelativeBinaryRange();
	return allMessagesInRange(messages, minValue, maxValue, CanBeZero::No);
}

bool MidiCCModeDetector::isMinMax1(const FixedSizeCircularBuffer<uint8_t, maxMessages> &messages) {
	auto [minValue, maxValue] = getMinMax1Values();
	return allMessagesExactly(messages, minValue, maxValue);
}

bool MidiCCModeDetector::isMinMax2(const FixedSizeCircularBuffer<uint8_t, maxMessages> &messages) {
	auto [minValue, maxValue] = getMinMax2Values();
	return allMessagesExactly(messages, minValue, maxValue);
}

MidiCCModeDetector::MidiCCMode
	MidiCCModeDetector::detectCCMode(const FixedSizeCircularBuffer<uint8_t, maxMessages> &messages) {
	if (messages.empty()) {
		return MidiCCMode::Unknown;
	}

	if (isArturiaRelative1(messages)) {
		return MidiCCModeDetector::MidiCCMode::ArturiaRelative1;
	}
	if (isArturiaRelative2(messages)) {
		return MidiCCModeDetector::MidiCCMode::ArturiaRelative2;
	}
	if (isArturiaRelative3(messages)) {
		return MidiCCModeDetector::MidiCCMode::ArturiaRelative3;
	}
	if (isMinMax1(messages)) {
		return MidiCCModeDetector::MidiCCMode::MinMax1;
	}
	if (isMinMax2(messages)) {
		return MidiCCModeDetector::MidiCCMode::MinMax2;
	}
	if (isRelativeBinaryOffset(messages)) {
		return MidiCCModeDetector::MidiCCMode::RelativeBinaryOffset;
	}

	return MidiCCMode::Absolute;
}
