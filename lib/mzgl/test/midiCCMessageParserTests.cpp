#include "tests.h"
#include "midi/MidiMessage.h"
#include "midi/MidiCCMessageParser.h"

SCENARIO("MidiCCMessageParser converts strings", "[midiccparser]") {
	for (auto type: {
			 MidiCCMessageParser::ChangeType::NotApplicable,
			 MidiCCMessageParser::ChangeType::Positive,
			 MidiCCMessageParser::ChangeType::Negative,
			 MidiCCMessageParser::ChangeType::NoChange,
		 }) {
		WHEN("Converting " + MidiCCMessageParser::toString(type)) {
			bool noThrows = true;
			try {
				auto str = MidiCCMessageParser::toString(type);
				REQUIRE_FALSE(str.empty());
				REQUIRE(MidiCCMessageParser::toType(str) == type);
			} catch (...) {
				noThrows = false;
			}
			REQUIRE(noThrows);
		}
	}
	REQUIRE_THROWS(MidiCCMessageParser::toType("FOOBAR"));
}

SCENARIO("MidiCCMessageParser converts to value", "[midiccparser]") {
	static const std::vector<std::pair<MidiCCMessageParser::ChangeType, int>> types {
		{MidiCCMessageParser::ChangeType::NotApplicable, 0},
		{MidiCCMessageParser::ChangeType::Positive, 1},
		{MidiCCMessageParser::ChangeType::Negative, -1},
		{MidiCCMessageParser::ChangeType::NoChange, 0},
	};
	for (auto [type, value]: types) {
		WHEN("Converting " + MidiCCMessageParser::toString(type)) {
			bool noThrows = true;
			try {
				REQUIRE(MidiCCMessageParser::toValue(type) == value);
			} catch (...) {
				noThrows = false;
			}
			REQUIRE(noThrows);
		}
	}
}

SCENARIO("MidiCCMessageParser parses min max 1", "[midiccparser]") {
	GIVEN("A Parser") {
		MidiCCMessageParser parser;
		WHEN("It processes a positive message") {
			REQUIRE(parser.determineChange(MidiMessage::cc(1, 16, MidiCCModeDetector::getMinMax1Values().first),
										   MidiCCModeDetector::MidiCCMode::MinMax1)
					== MidiCCMessageParser::ChangeType::Positive);
		}
		WHEN("It processes a negative message") {
			REQUIRE(parser.determineChange(MidiMessage::cc(1, 16, MidiCCModeDetector::getMinMax1Values().second),
										   MidiCCModeDetector::MidiCCMode::MinMax1)
					== MidiCCMessageParser::ChangeType::Negative);
		}
	}
}

SCENARIO("MidiCCMessageParser parses min max 2", "[midiccparser]") {
	GIVEN("A Parser") {
		MidiCCMessageParser parser;
		WHEN("It processes a positive message") {
			REQUIRE(parser.determineChange(MidiMessage::cc(1, 16, MidiCCModeDetector::getMinMax2Values().first),
										   MidiCCModeDetector::MidiCCMode::MinMax2)
					== MidiCCMessageParser::ChangeType::Positive);
		}
		WHEN("It processes a negative message") {
			REQUIRE(parser.determineChange(MidiMessage::cc(1, 16, MidiCCModeDetector::getMinMax2Values().second),
										   MidiCCModeDetector::MidiCCMode::MinMax2)
					== MidiCCMessageParser::ChangeType::Negative);
		}
	}
}

SCENARIO("MidiCCMessageParser parses binary offset", "[midiccparser]") {
	GIVEN("A Parser") {
		MidiCCMessageParser parser;
		WHEN("It processes a positive message") {
			REQUIRE(
				parser.determineChange(MidiMessage::cc(1, 16, MidiCCModeDetector::getRelativeBinaryRange().first),
									   MidiCCModeDetector::MidiCCMode::RelativeBinaryOffset)
				== MidiCCMessageParser::ChangeType::Positive);
		}
		WHEN("It processes a negative message") {
			REQUIRE(
				parser.determineChange(MidiMessage::cc(1, 16, MidiCCModeDetector::getRelativeBinaryRange().second),
									   MidiCCModeDetector::MidiCCMode::RelativeBinaryOffset)
				== MidiCCMessageParser::ChangeType::Negative);
		}
		WHEN("It processes a zero message") {
			REQUIRE(parser.determineChange(MidiMessage::cc(1, 16, 64),
										   MidiCCModeDetector::MidiCCMode::RelativeBinaryOffset)
					== MidiCCMessageParser::ChangeType::NoChange);
		}
	}
}

SCENARIO("MidiCCMessageParser parses Arturia relative 1", "[midiccparser]") {
	GIVEN("A Parser") {
		MidiCCMessageParser parser;
		WHEN("It processes a positive message") {
			REQUIRE(parser.determineChange(
						MidiMessage::cc(1, 16, MidiCCModeDetector::getArturiaRelative1Range().second),
						MidiCCModeDetector::MidiCCMode::ArturiaRelative1)
					== MidiCCMessageParser::ChangeType::Positive);
		}
		WHEN("It processes a negative message") {
			REQUIRE(parser.determineChange(
						MidiMessage::cc(1, 16, MidiCCModeDetector::getArturiaRelative1Range().first),
						MidiCCModeDetector::MidiCCMode::ArturiaRelative1)
					== MidiCCMessageParser::ChangeType::Negative);
		}
		WHEN("It processes a zero message") {
			REQUIRE(
				parser.determineChange(MidiMessage::cc(1, 16, 0), MidiCCModeDetector::MidiCCMode::ArturiaRelative1)
				== MidiCCMessageParser::ChangeType::NoChange);
		}
	}
}

SCENARIO("MidiCCMessageParser parses Arturia relative 2", "[midiccparser]") {
	GIVEN("A Parser") {
		MidiCCMessageParser parser;
		WHEN("It processes a positive message") {
			REQUIRE(
				parser.determineChange(MidiMessage::cc(1, 16, 3), MidiCCModeDetector::MidiCCMode::ArturiaRelative2)
				== MidiCCMessageParser::ChangeType::Positive);
		}
		WHEN("It processes a negative message") {
			REQUIRE(parser.determineChange(MidiMessage::cc(1, 16, 125),
										   MidiCCModeDetector::MidiCCMode::ArturiaRelative2)
					== MidiCCMessageParser::ChangeType::Negative);
		}
		WHEN("It processes a zero message") {
			REQUIRE(
				parser.determineChange(MidiMessage::cc(1, 16, 0), MidiCCModeDetector::MidiCCMode::ArturiaRelative2)
				== MidiCCMessageParser::ChangeType::NoChange);
		}
	}
}

SCENARIO("MidiCCMessageParser parses Arturia relative 3", "[midiccparser]") {
	GIVEN("A Parser") {
		MidiCCMessageParser parser;
		WHEN("It processes a positive message") {
			REQUIRE(parser.determineChange(MidiMessage::cc(1, 16, 19),
										   MidiCCModeDetector::MidiCCMode::ArturiaRelative3)
					== MidiCCMessageParser::ChangeType::Positive);
		}
		WHEN("It processes a negative message") {
			REQUIRE(parser.determineChange(MidiMessage::cc(1, 16, 13),
										   MidiCCModeDetector::MidiCCMode::ArturiaRelative3)
					== MidiCCMessageParser::ChangeType::Negative);
		}
		WHEN("It processes a zero message") {
			REQUIRE(
				parser.determineChange(MidiMessage::cc(1, 16, 0), MidiCCModeDetector::MidiCCMode::ArturiaRelative3)
				== MidiCCMessageParser::ChangeType::NoChange);
		}
	}
}

SCENARIO("MidiCCMessageParser parses 2s complement", "[midiccparser]") {
	GIVEN("A Parser") {
		MidiCCMessageParser parser;
		WHEN("It processes a positive message") {
			REQUIRE(parser.determineChange(MidiMessage::cc(1, 16, 0x05),
										   MidiCCModeDetector::MidiCCMode::TwosComplement)
					== MidiCCMessageParser::ChangeType::Positive);
		}
		WHEN("It processes a negative message") {
			REQUIRE(parser.determineChange(MidiMessage::cc(1, 16, 0x7B),
										   MidiCCModeDetector::MidiCCMode::TwosComplement)
					== MidiCCMessageParser::ChangeType::Negative);
		}
	}
}

SCENARIO("MidiCCMessageParser parses signed bit 1", "[midiccparser]") {
	GIVEN("A Parser") {
		MidiCCMessageParser parser;
		WHEN("It processes a positive message") {
			REQUIRE(
				parser.determineChange(MidiMessage::cc(1, 16, 0x05), MidiCCModeDetector::MidiCCMode::SignedBit1)
				== MidiCCMessageParser::ChangeType::Positive);
		}
		WHEN("It processes a negative message") {
			REQUIRE(
				parser.determineChange(MidiMessage::cc(1, 16, 0x45), MidiCCModeDetector::MidiCCMode::SignedBit1)
				== MidiCCMessageParser::ChangeType::Negative);
		}
	}
}

SCENARIO("MidiCCMessageParser parses signed bit 2", "[midiccparser]") {
	GIVEN("A Parser") {
		MidiCCMessageParser parser;
		WHEN("It processes a positive message") {
			REQUIRE(
				parser.determineChange(MidiMessage::cc(1, 16, 0x45), MidiCCModeDetector::MidiCCMode::SignedBit2)
				== MidiCCMessageParser::ChangeType::Positive);
		}
		WHEN("It processes a negative message") {
			REQUIRE(
				parser.determineChange(MidiMessage::cc(1, 16, 0x05), MidiCCModeDetector::MidiCCMode::SignedBit2)
				== MidiCCMessageParser::ChangeType::Negative);
		}
	}
}