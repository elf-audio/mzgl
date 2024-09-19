#include "tests.h"
#include "midi/MidiMessage.h"

SCENARIO("MidiMessage correctly identifies Note On events", "[midimessage]") {
	GIVEN("A MidiMessage representing a Note On event with non-zero velocity") {
		uint8_t noteOnMessage[] = {0x90, 0x3C, 0x40};
		MidiMessage message(noteOnMessage, 3);

		WHEN("The message is queried for a Note On event") {
			auto isNoteOn = message.isNoteOn();

			THEN("It should return true") {
				REQUIRE(isNoteOn);
			}
		}
	}

	GIVEN("A MidiMessage representing a Note On event with zero velocity") {
		uint8_t noteOnZeroVelocity[] = {0x90, 0x3C, 0x00};
		MidiMessage message(noteOnZeroVelocity, 3);

		WHEN("The message is queried for a Note Off event") {
			auto isNoteOff = message.isNoteOff();

			THEN("It should return true as Note On with zero velocity is treated as Note Off") {
				REQUIRE(isNoteOff);
			}
		}
	}
}

SCENARIO("MidiMessage correctly identifies Note Off events", "[midimessage]") {
	GIVEN("A MidiMessage representing a Note Off event") {
		uint8_t noteOffMessage[] = {0x80, 0x3C, 0x40};
		MidiMessage message(noteOffMessage, 3);

		WHEN("The message is queried for a Note Off event") {
			auto isNoteOff = message.isNoteOff();

			THEN("It should return true") {
				REQUIRE(isNoteOff);
			}
		}
	}
}

SCENARIO("MidiMessage correctly identifies Pitch Bend events", "[midimessage]") {
	GIVEN("A MidiMessage representing a Pitch Bend event") {
		uint8_t pitchBendMessage[] = {0xE0, 0x00, 0x40};
		MidiMessage message(pitchBendMessage, 3);

		WHEN("The message is queried for a Pitch Bend event") {
			auto isPitchBend = message.isPitchBend();

			THEN("It should return true") {
				REQUIRE(isPitchBend);
			}

			AND_WHEN("The pitch bend value is converted to a float") {
				float pitchBendValue = message.getPitchBend();

				THEN("The pitch bend value should be 0.0, as it is in the center position") {
					REQUIRE(pitchBendValue == Approx(0.0f));
				}
			}
		}
	}
}

SCENARIO("MidiMessage correctly identifies Control Change events", "[midimessage]") {
	GIVEN("A MidiMessage representing a Control Change (Mod Wheel)") {
		uint8_t ccMessage[] = {0xB0, 0x01, 0x40};
		MidiMessage message(ccMessage, 3);

		WHEN("The message is queried for a Control Change event") {
			auto isCC		= message.isCC();
			auto isModWheel = message.isModWheel();

			THEN("It should return true for CC and Mod Wheel") {
				REQUIRE(isCC);
				REQUIRE(isModWheel);
			}
		}
	}
}

SCENARIO("MidiMessage correctly identifies System Exclusive (Sysex) events", "[midimessage]") {
	GIVEN("A MidiMessage representing a Sysex event") {
		std::vector<uint8_t> sysexData = {0xF0, 0x7E, 0x00, 0xF7};
		MidiMessage message(sysexData);

		WHEN("The message is queried for a Sysex event") {
			auto isSysex = message.isSysex();

			THEN("It should return true") {
				REQUIRE(isSysex);
			}
		}
	}
}

SCENARIO("MidiMessage correctly identifies Song Position Pointer events", "[midimessage]") {
	GIVEN("A MidiMessage representing a Song Position Pointer event") {
		uint8_t sppMessage[] = {0xF2, 0x00, 0x04};
		MidiMessage message(sppMessage, 3);

		WHEN("The message is queried for a Song Position Pointer event") {
			auto isSPP = message.isSongPositionPointer();

			THEN("It should return true") {
				REQUIRE(isSPP);
			}

			AND_WHEN("The song position is converted to a double") {
				double songPosition = message.getSongPosition();

				THEN("The song position should be 128 beats (512 16th notes)") {
					REQUIRE(songPosition == Approx(128.0));
				}
			}
		}
	}
}

SCENARIO("MidiMessage handles static constructors correctly", "[midimessage]") {
	GIVEN("A Note On message is created with the static constructor") {
		MidiMessage message = MidiMessage::noteOn(1, 60, 100);

		WHEN("The message is queried for Note On and its properties") {
			auto isNoteOn = message.isNoteOn();
			auto channel  = message.getChannel();
			auto pitch	  = message.getPitch();
			auto velocity = message.getVelocity();

			THEN("It should return true for Note On and have correct pitch and velocity") {
				REQUIRE(isNoteOn);
				REQUIRE(channel == 1);
				REQUIRE(pitch == 60);
				REQUIRE(velocity == 100);
			}
		}
	}

	GIVEN("An All Notes Off message is created with the static constructor") {
		MidiMessage message = MidiMessage::allNotesOff();

		WHEN("The message is queried for an All Notes Off event") {
			auto isAllNotesOff = message.isAllNotesOff();

			THEN("It should return true") {
				REQUIRE(isAllNotesOff);
			}
		}
	}
}

SCENARIO("MidiMessage correctly parses raw byte arrays", "[midimessage]") {
	GIVEN("A raw byte array representing a Note On message") {
		const uint8_t rawBytes[] = {0x90, 0x3C, 0x40};

		WHEN("The MidiMessage is constructed from the byte array") {
			MidiMessage message(rawBytes, 3);

			THEN("It should correctly identify as a Note On event") {
				REQUIRE(message.isNoteOn());
			}

			AND_THEN("It should have correct pitch and velocity values") {
				REQUIRE(message.getPitch() == 60);
				REQUIRE(message.getVelocity() == 64);
			}
		}
	}
}

SCENARIO("MidiMessage correctly identifies Start events", "[midimessage]") {
	GIVEN("A MidiMessage representing a Start event") {
		uint8_t startMessage[] = {0xFA};
		MidiMessage message(startMessage, 1);

		WHEN("The message is queried for a Start event") {
			auto isStart = message.isStart();

			THEN("It should return true") {
				REQUIRE(isStart);
			}
		}
	}

	GIVEN("A MidiMessage representing a non-Start event") {
		uint8_t nonStartMessage[] = {0xF8};
		MidiMessage message(nonStartMessage, 1);

		WHEN("The message is queried for a Start event") {
			auto isStart = message.isStart();

			THEN("It should return false") {
				REQUIRE_FALSE(isStart);
			}
		}
	}
}

SCENARIO("MidiMessage correctly identifies Continue events", "[midimessage]") {
	GIVEN("A MidiMessage representing a Continue event") {
		uint8_t continueMessage[] = {0xFB};
		MidiMessage message(continueMessage, 1);

		WHEN("The message is queried for a Continue event") {
			auto isContinue = message.isContinue();

			THEN("It should return true") {
				REQUIRE(isContinue);
			}
		}
	}

	GIVEN("A MidiMessage representing a non-Stop event") {
		uint8_t nonContinueMessage[] = {0xFA};
		MidiMessage message(nonContinueMessage, 1);

		WHEN("The message is queried for a Stop event") {
			auto isContinue = message.isContinue();

			THEN("It should return false") {
				REQUIRE_FALSE(isContinue);
			}
		}
	}
}

SCENARIO("MidiMessage correctly identifies Stop events", "[midimessage]") {
	GIVEN("A MidiMessage representing a Stop event") {
		uint8_t stopMessage[] = {0xFC};
		MidiMessage message(stopMessage, 1);

		WHEN("The message is queried for a Stop event") {
			auto isStop = message.isStop();

			THEN("It should return true") {
				REQUIRE(isStop);
			}
		}
	}

	GIVEN("A MidiMessage representing a non-Stop event") {
		uint8_t nonStopMessage[] = {0xFA};
		MidiMessage message(nonStopMessage, 1);

		WHEN("The message is queried for a Stop event") {
			auto isStop = message.isStop();

			THEN("It should return false") {
				REQUIRE_FALSE(isStop);
			}
		}
	}
}

SCENARIO("MidiMessages are correctly stored in and extracted from a vector", "[midimessage]") {
	GIVEN("A vector of MidiMessage objects with various messages") {
		std::vector<MidiMessage> midiMessages;

		MidiMessage noteOn		 = MidiMessage::noteOn(1, 60, 100);
		MidiMessage noteOff		 = MidiMessage::noteOff(1, 60);
		MidiMessage startMessage = MidiMessage::songStart();
		MidiMessage stopMessage	 = MidiMessage::songStop();

		midiMessages.push_back(noteOn);
		midiMessages.push_back(noteOff);
		midiMessages.push_back(startMessage);
		midiMessages.push_back(stopMessage);

		WHEN("The MidiMessages are extracted from the vector") {
			MidiMessage extractedNoteOn	 = midiMessages[0];
			MidiMessage extractedNoteOff = midiMessages[1];
			MidiMessage extractedStart	 = midiMessages[2];
			MidiMessage extractedStop	 = midiMessages[3];

			THEN("They should be the same as the original MidiMessages") {
				REQUIRE(extractedNoteOn == noteOn);
				REQUIRE(extractedNoteOff == noteOff);
				REQUIRE(extractedStart == startMessage);
				REQUIRE(extractedStop == stopMessage);
			}

			THEN("The Note On message should still be identified correctly") {
				REQUIRE(extractedNoteOn.isNoteOn());
			}

			THEN("The Note Off message should still be identified correctly") {
				REQUIRE(extractedNoteOff.isNoteOff());
			}

			THEN("The Start message should still be identified correctly") {
				REQUIRE(extractedStart.isStart());
			}

			THEN("The Stop message should still be identified correctly") {
				REQUIRE(extractedStop.isStop());
			}
		}
	}
}