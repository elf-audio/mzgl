#pragma once

#include <vector>
#include "MidiMessage.h"
#include <functional>

/**
 * Turns a raw stream of MIDI bytes into complete messages, handling:
 *  - messages split across parse() calls
 *  - running status (data bytes with the status byte omitted)
 *  - system realtime bytes (0xF8..0xFF) interleaved inside another message
 *  - a status byte interrupting an incomplete message (the partial one is dropped)
 * One instance per input port - the running status is per-stream state.
 */
class MidiMessageParser {
public:
	using MidiByte = unsigned char;

	struct MidiData {
		const std::vector<MidiByte> &data;
		uint64_t timestamp;
	};

	explicit MidiMessageParser(const std::function<void(const MidiData &data)> &onDataReady);
	void parse(const std::vector<MidiByte> &midiData, uint64_t timestamp, int32_t deviceId, int32_t portId);
	void parse(const MidiByte *midiData, size_t length, uint64_t timestamp);

private:
	void parseByte(MidiByte byte, uint64_t timestamp);
	void emitCurrent(uint64_t timestamp);

	std::vector<MidiByte> currentData;
	std::vector<MidiByte> realtimeData;
	MidiByte runningStatus = 0;
	std::function<void(const MidiData &data)> dataReadyCallback;
};
