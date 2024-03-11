#pragma once

#include <vector>
#include "MidiMessage.h"
#include <functional>

class MidiMessageParser {
public:
	using MidiByte = unsigned char;

	struct MidiData {
		const std::vector<MidiByte> &data;
		uint64_t timestamp;
	};

	explicit MidiMessageParser(const std::function<void(const MidiData &data)> &onDataReady);
	void parse(const std::vector<MidiByte> &midiData, uint64_t timestamp, int32_t deviceId, int32_t portId);

private:
	static constexpr MidiByte statusByte = 0x80;

	static bool isStatusByte(MidiByte byte);
	[[nodiscard]] bool currentDataIsStatusByte() const;
	[[nodiscard]] bool hasNoCurrentDataOrOnlyAStatusByte() const;
	void emitCurrent(uint64_t timestamp);
	void emitCurrentIfF6OrHigher(uint64_t timestamp);
	void handleStatusByte(MidiByte byte, uint64_t timestamp);
	void handleNormalData(MidiByte byte, uint64_t timestamp);

	std::vector<unsigned char> currentData;
	std::function<void(const MidiData &data)> dataReadyCallback;
};
