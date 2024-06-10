#include "tests.h"
#include "chords.h"
#define C  0
#define Cs 1
#define D  2
#define Ds 3
#define E  4
#define F  5
#define Fs 6
#define G  7
#define Gs 8
#define A  9
#define As 10
#define B  11
TEST_CASE("chordNameTests", "[core][chordNameTests]") {
	REQUIRE(notesToChordName({C, E, G}) == "C");
	REQUIRE(notesToChordName({C, E, G, B}) == "Cmaj7");
	REQUIRE(notesToChordName({D, F, C}) == "Dm7");
	REQUIRE(notesToChordName({C, D, E, As}) == "C9");
	REQUIRE(notesToChordName({G, B, D, Fs, A, E}) == "Gmaj13");
	REQUIRE(notesToChordName({Ds, G, B}) == "D#aug");
	REQUIRE(notesToChordName({A, C, E, Gs}) == "AmM7");
	REQUIRE(notesToChordName({F, A, Cs, Ds}) == "F7#5");
	REQUIRE(notesToChordName({C, Ds, G, D}) == "Cmadd9");
}