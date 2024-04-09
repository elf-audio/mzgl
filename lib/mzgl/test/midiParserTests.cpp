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

SCENARIO("Delimits multiple note ons off properly", "[midi-parser]") {
	GIVEN("Some midi data and a parser") {
		const std::vector<std::vector<uint8_t>> originalMessages {
			// note on, note off
			{0x92, 0x30, 0x40},
			{0x82, 0x30, 0x00},

			// 4 cc messages
			{0xB0, 0x0A, 0x20},
			{0xB0, 0x0A, 0x40},
			{0xB0, 0x0A, 0x60},
			{0xB0, 0x0A, 0x7F},
			// midi start
			{0xFA},
			/// random sysex
			{0xF0, 0x25, 0xC1, 0x50, 0xF4, 0xC5, 0xF7},
			// midi note off again
			{0x82, 0x30, 0x00},
			// sysex
			{0xF0, 0x25, 0xC1, 0xF7},

			{0x92, 0x30, 0x40},
		};

		// concatenate all the vectors into one
		std::vector<uint8_t> concatenatedMessages;
		for (auto &msg: originalMessages) {
			concatenatedMessages.insert(concatenatedMessages.end(), msg.begin(), msg.end());
		}

		const std::vector<std::vector<MidiMessageParser::MidiByte>> msgs {concatenatedMessages};

		int messageCounter = 0;
		MidiMessageParser parser {[&](const MidiMessageParser::MidiData &data) {
			REQUIRE(data.data == originalMessages[messageCounter]);
			messageCounter++;
		}};

		WHEN("The messages are sent") {
			for (auto &msg: msgs) {
				parser.parse(msg, 666, 0, 0);
			}
			THEN("We should have got the callback") {
				REQUIRE(messageCounter == originalMessages.size());
			}
		}
	}
}