#include "tests.h"
#include "midi/MidiMessage.h"
#include "midi/MidiCCModeDetector.h"

SCENARIO("MidiCCModeDetector converts strings", "[midiccmode]") {
	for (auto mode: {
			 MidiCCModeDetector::MidiCCMode::Unknown,
			 MidiCCModeDetector::MidiCCMode::Absolute,
			 MidiCCModeDetector::MidiCCMode::TwosComplement,
			 MidiCCModeDetector::MidiCCMode::RelativeBinaryOffset,
			 MidiCCModeDetector::MidiCCMode::SignedBit1,
			 MidiCCModeDetector::MidiCCMode::SignedBit2,
			 MidiCCModeDetector::MidiCCMode::ArturiaRelative1,
			 MidiCCModeDetector::MidiCCMode::ArturiaRelative2,
			 MidiCCModeDetector::MidiCCMode::ArturiaRelative3,
		 }) {
		WHEN("Converting " + MidiCCModeDetector::toString(mode)) {
			bool noThrows = true;
			try {
				auto str = MidiCCModeDetector::toString(mode);
				REQUIRE_FALSE(str.empty());
				REQUIRE(MidiCCModeDetector::toMode(str) == mode);
			} catch (...) {
				noThrows = false;
			}
			REQUIRE(noThrows);
		}
	}
	REQUIRE_THROWS(MidiCCModeDetector::toMode("FOOBAR"));
}

SCENARIO("MidiCCModeDetector processes MIDI CC messages and detects absolute mode", "[midiccmode]") {
	GIVEN("A MidiCCModeDetector") {
		MidiCCModeDetector detector;
		WHEN("Processing a series of absolute CC messages") {
			static constexpr auto midiCCChannel = 1;
			static constexpr auto midiCCIndex	= 16;
			for (int value = 0; value < 127; ++value) {
				detector.process(MidiMessage::cc(midiCCChannel, midiCCIndex, value));
			}
			THEN("It should detect the mode as Absolute") {
				auto mode = detector.getDetectedMode(midiCCIndex);
				REQUIRE(mode == MidiCCModeDetector::MidiCCMode::Absolute);
			}
		}
	}
}

SCENARIO("MidiCCModeDetector processes MIDI CC messages and detects Arturia relative 1", "[midiccmode]") {
	GIVEN("A MidiCCModeDetector") {
		MidiCCModeDetector detector;
		WHEN("Processing a series of absolute CC messages") {
			static constexpr auto midiCCChannel = 1;
			static constexpr auto midiCCIndex	= 16;
			detector.process(MidiMessage::cc(midiCCChannel, midiCCIndex, 61));
			detector.process(MidiMessage::cc(midiCCChannel, midiCCIndex, 0));
			detector.process(MidiMessage::cc(midiCCChannel, midiCCIndex, 63));
			detector.process(MidiMessage::cc(midiCCChannel, midiCCIndex, 0));
			detector.process(MidiMessage::cc(midiCCChannel, midiCCIndex, 67));
			detector.process(MidiMessage::cc(midiCCChannel, midiCCIndex, 0));
			THEN("It should detect the mode as Arturia relative 1") {
				auto mode = detector.getDetectedMode(midiCCIndex);
				REQUIRE(mode == MidiCCModeDetector::MidiCCMode::ArturiaRelative1);
			}
		}
	}
}

SCENARIO("MidiCCModeDetector processes MIDI CC messages and detects Arturia relative 2", "[midiccmode]") {
	GIVEN("A MidiCCModeDetector") {
		MidiCCModeDetector detector;
		WHEN("Processing a series of absolute CC messages") {
			static constexpr auto midiCCChannel = 1;
			static constexpr auto midiCCIndex	= 16;
			detector.process(MidiMessage::cc(midiCCChannel, midiCCIndex, 1));
			detector.process(MidiMessage::cc(midiCCChannel, midiCCIndex, 0));
			detector.process(MidiMessage::cc(midiCCChannel, midiCCIndex, 2));
			detector.process(MidiMessage::cc(midiCCChannel, midiCCIndex, 0));
			detector.process(MidiMessage::cc(midiCCChannel, midiCCIndex, 3));
			detector.process(MidiMessage::cc(midiCCChannel, midiCCIndex, 0));
			detector.process(MidiMessage::cc(midiCCChannel, midiCCIndex, 125));
			detector.process(MidiMessage::cc(midiCCChannel, midiCCIndex, 0));
			detector.process(MidiMessage::cc(midiCCChannel, midiCCIndex, 126));
			detector.process(MidiMessage::cc(midiCCChannel, midiCCIndex, 0));
			detector.process(MidiMessage::cc(midiCCChannel, midiCCIndex, 127));
			THEN("It should detect the mode as Arturia relative 2") {
				auto mode = detector.getDetectedMode(midiCCIndex);
				REQUIRE(mode == MidiCCModeDetector::MidiCCMode::ArturiaRelative2);
			}
		}
	}
}

SCENARIO("MidiCCModeDetector processes MIDI CC messages and detects Arturia relative 3", "[midiccmode]") {
	GIVEN("A MidiCCModeDetector") {
		MidiCCModeDetector detector;
		WHEN("Processing a series of absolute CC messages") {
			static constexpr auto midiCCChannel = 1;
			static constexpr auto midiCCIndex	= 16;
			detector.process(MidiMessage::cc(midiCCChannel, midiCCIndex, 15));
			detector.process(MidiMessage::cc(midiCCChannel, midiCCIndex, 0));
			detector.process(MidiMessage::cc(midiCCChannel, midiCCIndex, 14));
			detector.process(MidiMessage::cc(midiCCChannel, midiCCIndex, 0));
			detector.process(MidiMessage::cc(midiCCChannel, midiCCIndex, 13));
			detector.process(MidiMessage::cc(midiCCChannel, midiCCIndex, 0));
			detector.process(MidiMessage::cc(midiCCChannel, midiCCIndex, 19));
			detector.process(MidiMessage::cc(midiCCChannel, midiCCIndex, 0));
			detector.process(MidiMessage::cc(midiCCChannel, midiCCIndex, 17));
			detector.process(MidiMessage::cc(midiCCChannel, midiCCIndex, 0));
			detector.process(MidiMessage::cc(midiCCChannel, midiCCIndex, 18));
			THEN("It should detect the mode as Arturia relative 3") {
				auto mode = detector.getDetectedMode(midiCCIndex);
				REQUIRE(mode == MidiCCModeDetector::MidiCCMode::ArturiaRelative3);
			}
		}
	}
}

SCENARIO("MidiCCModeDetector processes MIDI CC messages and detects Relative Binary Offset", "[midiccmode]") {
	GIVEN("A MidiCCModeDetector") {
		MidiCCModeDetector detector;
		WHEN("Processing a series of absolute CC messages") {
			static constexpr auto midiCCChannel = 1;
			static constexpr auto midiCCIndex	= 16;
			detector.process(MidiMessage::cc(midiCCChannel, midiCCIndex, 63));
			detector.process(MidiMessage::cc(midiCCChannel, midiCCIndex, 64));
			detector.process(MidiMessage::cc(midiCCChannel, midiCCIndex, 65));
			detector.process(MidiMessage::cc(midiCCChannel, midiCCIndex, 65));
			detector.process(MidiMessage::cc(midiCCChannel, midiCCIndex, 65));
			detector.process(MidiMessage::cc(midiCCChannel, midiCCIndex, 65));
			detector.process(MidiMessage::cc(midiCCChannel, midiCCIndex, 65));
			detector.process(MidiMessage::cc(midiCCChannel, midiCCIndex, 63));
			THEN("It should detect the mode as Relative Binary Offset") {
				auto mode = detector.getDetectedMode(midiCCIndex);
				REQUIRE(mode == MidiCCModeDetector::MidiCCMode::RelativeBinaryOffset);
			}
		}
	}
}

SCENARIO("MidiCCModeDetector processes MIDI CC messages and detects Min Max 1", "[midiccmode]") {
	GIVEN("A MidiCCModeDetector") {
		MidiCCModeDetector detector;
		WHEN("Processing a series of absolute CC messages") {
			static constexpr auto midiCCChannel = 1;
			static constexpr auto midiCCIndex	= 16;
			detector.process(MidiMessage::cc(midiCCChannel, midiCCIndex, 1));
			detector.process(MidiMessage::cc(midiCCChannel, midiCCIndex, 127));
			detector.process(MidiMessage::cc(midiCCChannel, midiCCIndex, 127));
			detector.process(MidiMessage::cc(midiCCChannel, midiCCIndex, 127));
			detector.process(MidiMessage::cc(midiCCChannel, midiCCIndex, 1));
			detector.process(MidiMessage::cc(midiCCChannel, midiCCIndex, 1));
			detector.process(MidiMessage::cc(midiCCChannel, midiCCIndex, 127));
			detector.process(MidiMessage::cc(midiCCChannel, midiCCIndex, 127));
			THEN("It should detect the mode as Min Max 1") {
				auto mode = detector.getDetectedMode(midiCCIndex);
				REQUIRE(mode == MidiCCModeDetector::MidiCCMode::MinMax1);
			}
		}
	}
}

SCENARIO("MidiCCModeDetector processes MIDI CC messages and detects Min Max 2", "[midiccmode]") {
	GIVEN("A MidiCCModeDetector") {
		MidiCCModeDetector detector;
		WHEN("Processing a series of absolute CC messages") {
			static constexpr auto midiCCChannel = 1;
			static constexpr auto midiCCIndex	= 16;
			detector.process(MidiMessage::cc(midiCCChannel, midiCCIndex, 63));
			detector.process(MidiMessage::cc(midiCCChannel, midiCCIndex, 65));
			detector.process(MidiMessage::cc(midiCCChannel, midiCCIndex, 65));
			detector.process(MidiMessage::cc(midiCCChannel, midiCCIndex, 63));
			detector.process(MidiMessage::cc(midiCCChannel, midiCCIndex, 65));
			detector.process(MidiMessage::cc(midiCCChannel, midiCCIndex, 63));
			detector.process(MidiMessage::cc(midiCCChannel, midiCCIndex, 63));
			detector.process(MidiMessage::cc(midiCCChannel, midiCCIndex, 65));
			THEN("It should detect the mode as Min Max 2") {
				auto mode = detector.getDetectedMode(midiCCIndex);
				REQUIRE(mode == MidiCCModeDetector::MidiCCMode::MinMax2);
			}
		}
	}
}

SCENARIO("MidiCCModeDetector processor handles errors", "[midiccmode]") {
	GIVEN("A detector") {
		MidiCCModeDetector detector;

		WHEN("Requesting the detected mode for an invalid CC index") {
			THEN("It should throw an out_of_range exception") {
				REQUIRE_THROWS_AS(detector.getDetectedMode(128), std::out_of_range);
			}
		}

		WHEN("Processing non-CC MIDI messages") {
			auto msg = MidiMessage::noteOn(1, 60, 100);
			detector.process(msg);

			THEN("It should ignore them and maintain default modes") {
				auto mode = detector.getDetectedMode(0);
				REQUIRE(mode == MidiCCModeDetector::MidiCCMode::Unknown);
			}
		}
	}
}