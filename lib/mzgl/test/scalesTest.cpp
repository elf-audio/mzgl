
#include "tests.h"
#include "scales.h"

TEST_CASE("Scales convert properly when pitch is default") {
	REQUIRE(midiNoteNumToString(0) == "C-2");
	REQUIRE(midiNoteNumToString(60) == "C3");

	REQUIRE(stringToMidiNoteNum("C3") == 60);
	REQUIRE(stringToMidiNoteNum("sdfkjsfdkj") == -1);
	REQUIRE(stringToMidiNoteNum("C-2") == 0);
	REQUIRE(stringToMidiNoteNum("F#5") == 90);
	REQUIRE(stringToMidiNoteNum("Bb-1") == -1);
	REQUIRE(stringToMidiNoteNum("B2") == 59);
	REQUIRE(stringToMidiNoteNum("2B") == -1);
	REQUIRE(stringToMidiNoteNum("") == -1);
	for (int i = 0; i < 128; i++) {
		REQUIRE(stringToMidiNoteNum(midiNoteNumToString(i)) == i);
	}
}
