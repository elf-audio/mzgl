#include "tests.h"
#include "MidiMessageParser.h"

SCENARIO("Input sysex is parsed properly", "[midi-parser]") {
	GIVEN("Some sysex and a parser") {
		const std::vector<MidiMessageParser::MidiByte> sysex {
			0xF0, 0x41, 0x10, 0x00, 0x00, 0x00, 0x00, 0x08, 0x12, 0x02, 0x00, 0x00, 0x02, 0x0F, 0x08, 0x65, 0xF7};

		bool gotCallback = false;
		MidiMessageParser parser {[&](const MidiMessageParser::MidiData &data) {
			REQUIRE(data.data == sysex);
			REQUIRE(data.timestamp == 666);
			gotCallback = true;
		}};

		WHEN("The data is parsed") {
			parser.parse(sysex, 666, 0, 0);
			THEN("We should have got the callback") {
				REQUIRE(gotCallback);
			}
		}
	}
}
SCENARIO("Input sysex is parsed properly in parts", "[midi-parser]") {
	GIVEN("Some sysex and a parser") {
		const std::vector<MidiMessageParser::MidiByte> sysex1 {
			0xF0, 0x41, 0x10, 0x00, 0x00, 0x00, 0x00, 0x08, 0x12};
		const std::vector<MidiMessageParser::MidiByte> sysex2 {0x02, 0x00, 0x00, 0x02, 0x0F, 0x08, 0x65, 0xF7};
		std::vector<MidiMessageParser::MidiByte> combinedSysex;
		combinedSysex.insert(combinedSysex.end(), sysex1.begin(), sysex1.end());
		combinedSysex.insert(combinedSysex.end(), sysex2.begin(), sysex2.end());

		bool gotCallback = false;
		MidiMessageParser parser {[&](const MidiMessageParser::MidiData &data) {
			REQUIRE(data.data == combinedSysex);
			REQUIRE(data.timestamp == 666);
			gotCallback = true;
		}};

		WHEN("The data is parsed in parts") {
			parser.parse(sysex1, 666, 0, 0);
			parser.parse(sysex2, 666, 0, 0);
			THEN("We should have got the callback") {
				REQUIRE(gotCallback);
			}
		}
	}
}
