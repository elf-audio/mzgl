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

SCENARIO("Running status is handled correctly for simultaneous note-ons", "[midi-parser]") {
	GIVEN("Three note-on events with running status (no repeated status byte)") {
		// This is what Android delivers when 3 notes are pressed simultaneously:
		// First note has status byte, subsequent notes omit it (running status)
		const std::vector<MidiMessageParser::MidiByte> runningStatusData {
			0x90,
			0x3C,
			0x7F, // note on C4
			0x3E,
			0x7F, // note on D4 (running status - no 0x90)
			0x40,
			0x7F, // note on E4 (running status - no 0x90)
		};

		const std::vector<std::vector<MidiMessageParser::MidiByte>> expectedMessages {
			{0x90, 0x3C, 0x7F},
			{0x90, 0x3E, 0x7F},
			{0x90, 0x40, 0x7F},
		};

		int messageCounter = 0;
		MidiMessageParser parser {[&](const MidiMessageParser::MidiData &data) {
			REQUIRE(messageCounter < expectedMessages.size());
			REQUIRE(data.data == expectedMessages[messageCounter]);
			messageCounter++;
		}};

		WHEN("The data is parsed in one chunk") {
			parser.parse(runningStatusData, 666, 0, 0);
			THEN("All three note-on messages should be emitted") {
				REQUIRE(messageCounter == 3);
			}
		}
	}
}

SCENARIO("Running status works across separate parse calls", "[midi-parser]") {
	GIVEN("Note events split across multiple parse calls with running status") {
		int messageCounter = 0;
		const std::vector<std::vector<MidiMessageParser::MidiByte>> expectedMessages {
			{0x90, 0x3C, 0x7F},
			{0x90, 0x3E, 0x7F},
		};

		MidiMessageParser parser {[&](const MidiMessageParser::MidiData &data) {
			REQUIRE(messageCounter < expectedMessages.size());
			REQUIRE(data.data == expectedMessages[messageCounter]);
			messageCounter++;
		}};

		WHEN("First message has status, second uses running status in separate call") {
			parser.parse({0x90, 0x3C, 0x7F}, 666, 0, 0);
			parser.parse({0x3E, 0x7F}, 666, 0, 0);
			THEN("Both messages should be emitted with the correct status byte") {
				REQUIRE(messageCounter == 2);
			}
		}
	}
}

SCENARIO("Running status does not apply after system messages", "[midi-parser]") {
	GIVEN("A note-on followed by a clock then running status data") {
		int messageCounter = 0;
		const std::vector<std::vector<MidiMessageParser::MidiByte>> expectedMessages {
			{0x90, 0x3C, 0x7F},
			{0xF8},
			{0x90, 0x3E, 0x7F},
		};

		MidiMessageParser parser {[&](const MidiMessageParser::MidiData &data) {
			REQUIRE(messageCounter < expectedMessages.size());
			REQUIRE(data.data == expectedMessages[messageCounter]);
			messageCounter++;
		}};

		WHEN("Clock interrupts running status") {
			// running status should still work because system realtime
			// messages don't cancel running status per MIDI spec
			parser.parse({0x90, 0x3C, 0x7F, 0xF8, 0x3E, 0x7F}, 666, 0, 0);
			THEN("All three messages should be emitted") {
				REQUIRE(messageCounter == 3);
			}
		}
	}
}

SCENARIO("Midi messages for aftertouch are valid with channels", "[midi-parser]") {
	GIVEN("Some midi data and a midi message") {
		for (int channel = 0; channel < 15; ++channel) {
			WHEN("Channel " + std::to_string(channel) + " is processed ") {
				MidiMessage message {{static_cast<unsigned char>(0xD0 + channel), 0x3C}};
				REQUIRE(message.getBytes().size() == 2);
				REQUIRE(message.getChannel() == channel + 1);
				REQUIRE(message.getBytes()[0] == 0xD0 + channel);
				REQUIRE(message.getBytes()[1] == 0x3C);
				REQUIRE(message.isChannelPressure());
			}
		}
	}
}

SCENARIO("Midi messages for polypressure are valid with channels", "[midi-parser]") {
	GIVEN("Some midi data and a midi message") {
		for (int channel = 0; channel < 15; ++channel) {
			WHEN("Channel " + std::to_string(channel) + " is processed ") {
				MidiMessage message {{static_cast<unsigned char>(0xA0 + channel), 0x3C, 0x64}};
				REQUIRE(message.getChannel() == channel + 1);
				REQUIRE(message.getBytes().size() == 3);
				REQUIRE(message.getBytes()[0] == 0xA0 + channel);
				REQUIRE(message.getBytes()[1] == 0x3C);
				REQUIRE(message.getBytes()[2] == 0x64);
				REQUIRE(message.isPolyPressure());
			}
		}
	}
}
SCENARIO("Running status alternating note on and note off, as a BLE keyboard sends it", "[midi-parser]") {
	GIVEN("A stream where only the first message carries a status byte") {
		// CoreMIDI hands Bluetooth MIDI packets through with running status intact,
		// so after the first note-on every following note is just two data bytes.
		const std::vector<std::vector<MidiMessageParser::MidiByte>> expectedMessages {
			{0x90, 0x3C, 0x64},
			{0x90, 0x3C, 0x00},
			{0x90, 0x3E, 0x50},
			{0x90, 0x3E, 0x00},
		};

		int messageCounter = 0;
		MidiMessageParser parser {[&](const MidiMessageParser::MidiData &data) {
			REQUIRE(messageCounter < expectedMessages.size());
			REQUIRE(data.data == expectedMessages[messageCounter]);
			messageCounter++;
		}};

		WHEN("Each note arrives in its own packet") {
			const std::vector<MidiMessageParser::MidiByte> p1 {0x90, 0x3C, 0x64};
			const std::vector<MidiMessageParser::MidiByte> p2 {0x3C, 0x00};
			const std::vector<MidiMessageParser::MidiByte> p3 {0x3E, 0x50};
			const std::vector<MidiMessageParser::MidiByte> p4 {0x3E, 0x00};
			parser.parse(p1.data(), p1.size(), 1);
			parser.parse(p2.data(), p2.size(), 2);
			parser.parse(p3.data(), p3.size(), 3);
			parser.parse(p4.data(), p4.size(), 4);
			THEN("All four messages come out with the status byte restored") {
				REQUIRE(messageCounter == 4);
			}
		}
	}
}

SCENARIO("Realtime bytes in the middle of a message don't corrupt it", "[midi-parser]") {
	GIVEN("A note-on with a clock byte between its data bytes") {
		const std::vector<std::vector<MidiMessageParser::MidiByte>> expectedMessages {
			{0xF8},
			{0x90, 0x3C, 0x7F},
			{0xFE},
			{0x90, 0x3E, 0x7F},
		};
		int messageCounter = 0;
		MidiMessageParser parser {[&](const MidiMessageParser::MidiData &data) {
			REQUIRE(messageCounter < expectedMessages.size());
			REQUIRE(data.data == expectedMessages[messageCounter]);
			messageCounter++;
		}};
		WHEN("The data is parsed") {
			parser.parse({0x90, 0x3C, 0xF8, 0x7F, 0x3E, 0xFE, 0x7F}, 666, 0, 0);
			THEN("Realtime bytes are emitted immediately and the notes stay intact") {
				REQUIRE(messageCounter == 4);
			}
		}
	}
}

SCENARIO("A status byte interrupting an incomplete message drops the partial one", "[midi-parser]") {
	GIVEN("A truncated note-on followed by a full one") {
		const std::vector<std::vector<MidiMessageParser::MidiByte>> expectedMessages {
			{0x90, 0x3E, 0x40},
		};
		int messageCounter = 0;
		MidiMessageParser parser {[&](const MidiMessageParser::MidiData &data) {
			REQUIRE(messageCounter < expectedMessages.size());
			REQUIRE(data.data == expectedMessages[messageCounter]);
			messageCounter++;
		}};
		WHEN("The data is parsed") {
			parser.parse({0x90, 0x3C, 0x90, 0x3E, 0x40}, 666, 0, 0);
			THEN("Only the complete message is emitted") {
				REQUIRE(messageCounter == 1);
			}
		}
	}
}

SCENARIO("Stray data bytes before any status byte are ignored", "[midi-parser]") {
	GIVEN("Data bytes with no running status established") {
		int messageCounter = 0;
		MidiMessageParser parser {[&](const MidiMessageParser::MidiData &data) {
			REQUIRE(data.data == std::vector<MidiMessageParser::MidiByte> {0xB0, 0x07, 0x40});
			messageCounter++;
		}};
		WHEN("The data is parsed") {
			parser.parse({0x3C, 0x7F, 0x3E, 0xB0, 0x07, 0x40}, 666, 0, 0);
			THEN("Only the real message is emitted") {
				REQUIRE(messageCounter == 1);
			}
		}
	}
}

SCENARIO("System common messages cancel running status", "[midi-parser]") {
	GIVEN("A note-on, then a song select, then bare data bytes") {
		const std::vector<std::vector<MidiMessageParser::MidiByte>> expectedMessages {
			{0x90, 0x3C, 0x7F},
			{0xF3, 0x05},
		};
		int messageCounter = 0;
		MidiMessageParser parser {[&](const MidiMessageParser::MidiData &data) {
			REQUIRE(messageCounter < expectedMessages.size());
			REQUIRE(data.data == expectedMessages[messageCounter]);
			messageCounter++;
		}};
		WHEN("The data is parsed") {
			parser.parse({0x90, 0x3C, 0x7F, 0xF3, 0x05, 0x3E, 0x7F}, 666, 0, 0);
			THEN("The trailing data bytes are not attached to the old status") {
				REQUIRE(messageCounter == 2);
			}
		}
	}
}
