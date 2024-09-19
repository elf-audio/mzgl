
#include "tests.h"
#include "scales.h"

TEST_CASE("Scales convert properly when pitch is default") {
	REQUIRE(midiNoteNumToString(0) == "C-2");
	REQUIRE(midiNoteNumToString(60) == "C3");
}
