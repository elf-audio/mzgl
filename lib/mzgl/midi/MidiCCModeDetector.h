#pragma once

#include <array>
#include <string>
#include <stdexcept>
#include <vector>
#include <cstdint>

#include "MidiMessage.h"
#include "FixedSizeCircularBuffer.h"

class MidiCCModeDetector {
public:
	enum class MidiCCMode {
		Unknown,
		/**
		 * This is the standard midi CC mode, goes from 0 - 127
		 */
		Absolute,
		/**
		 * This is described at:
		 * https://manual.ardour.org/using-control-surfaces/generic-midi/working-with-encoders/
		 * Note this uses the same range as Absolute range (0 - 127) and thus can't be automatically detected
		 */
		TwosComplement,
		/**
		 * These are used by Faderfox
		 * http://www.faderfox.de/PDF/UC4%20Manual%20V03.pdf
		 */
		MinMax1,
		MinMax2,
		/**
		 * This is described at:
		 * https://manual.ardour.org/using-control-surfaces/generic-midi/working-with-encoders/
		 * https://archive.steinberg.help/cubase_pro/v12/en/cubase_nuendo/topics/midi_remote/midi_remote_item_properties_r.html
		 * Note this uses the same range as Absolute range (0 - 127) and thus can't be automatically detected
		 */
		SignedBit1,
		SignedBit2,
		/**
		 * This is described variously at:
		 * https://www.ableton.com/en/manual/midi-and-key-remote-control/
		 * https://manual.ardour.org/using-control-surfaces/generic-midi/working-with-encoders/
		 * And is also called Enc 3FH/41H in some places
		 * Note - This is very close to MinMax2, except that it can send 64 to indicate no change
		 */
		RelativeBinaryOffset,
		/**
		 * These encodings are used in arturia devices. They are documented at:
		 * http://downloads.arturia.com/products/minilab-mkII/manual/MiniLabmkII_Manual_1_0_7_EN.pdf
		 * See page 34
		 */
		ArturiaRelative1,
		ArturiaRelative2,
		ArturiaRelative3,
	};

	MidiCCModeDetector();

	void process(const MidiMessage &message);
	[[nodiscard]] MidiCCMode getDetectedMode(uint8_t ccIndex) const;

	static std::string toString(MidiCCMode mode);
	static MidiCCMode toMode(const std::string &str);

	static std::pair<int, int> getMinMax1Values();
	static std::pair<int, int> getMinMax2Values();
	static std::pair<int, int> getRelativeBinaryRange();
	static std::pair<int, int> getArturiaRelative1Range();

private:
	static constexpr size_t maxMessages = 10;
	static constexpr size_t numberOfCCs = 128;

	struct CCInfo {
		FixedSizeCircularBuffer<uint8_t, maxMessages> buffer;
		MidiCCMode mode = MidiCCMode::Unknown;
	};

	std::array<CCInfo, numberOfCCs> ccInfos;

	static MidiCCMode detectCCMode(const FixedSizeCircularBuffer<uint8_t, maxMessages> &messages);

	enum class CanBeZero { Must, Yes, No };
	static bool allMessagesInRange(const FixedSizeCircularBuffer<uint8_t, maxMessages> &messages,
								   int min,
								   int max,
								   CanBeZero canBeZero);
	static bool allMessagesInRange(const FixedSizeCircularBuffer<uint8_t, maxMessages> &messages,
								   int range1Min,
								   int range1Max,
								   int range2Min,
								   int range2Max,
								   CanBeZero canBeZero);
	static bool
		allMessagesExactly(const FixedSizeCircularBuffer<uint8_t, maxMessages> &messages, int min, int max);

	static bool isArturiaRelative1(const FixedSizeCircularBuffer<uint8_t, maxMessages> &messages);
	static bool isArturiaRelative2(const FixedSizeCircularBuffer<uint8_t, maxMessages> &messages);
	static bool isArturiaRelative3(const FixedSizeCircularBuffer<uint8_t, maxMessages> &messages);
	static bool isRelativeBinaryOffset(const FixedSizeCircularBuffer<uint8_t, maxMessages> &messages);
	static bool isMinMax1(const FixedSizeCircularBuffer<uint8_t, maxMessages> &messages);
	static bool isMinMax2(const FixedSizeCircularBuffer<uint8_t, maxMessages> &messages);
};
