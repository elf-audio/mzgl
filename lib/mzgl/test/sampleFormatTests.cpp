#include "tests.h"

#include "SampleFormat.h"

SCENARIO("bytesPerSample returns the correct width for every SampleFormat", "[sample-format]") {
	GIVEN("each SampleFormat enum value") {
		THEN("I8 is 1 byte per sample") {
			REQUIRE(bytesPerSample(SampleFormat::I8) == 1);
		}
		THEN("I16 is 2 bytes per sample") {
			REQUIRE(bytesPerSample(SampleFormat::I16) == 2);
		}
		THEN("I24 is 3 bytes per sample (packed little-endian)") {
			REQUIRE(bytesPerSample(SampleFormat::I24) == 3);
		}
		THEN("F32 is 4 bytes per sample") {
			REQUIRE(bytesPerSample(SampleFormat::F32) == 4);
		}
	}
}

SCENARIO("SampleFormat enum values are distinct and stable", "[sample-format]") {
	GIVEN("the four supported formats") {
		WHEN("compared pairwise") {
			THEN("none collide") {
				REQUIRE(SampleFormat::I8 != SampleFormat::I16);
				REQUIRE(SampleFormat::I8 != SampleFormat::I24);
				REQUIRE(SampleFormat::I8 != SampleFormat::F32);
				REQUIRE(SampleFormat::I16 != SampleFormat::I24);
				REQUIRE(SampleFormat::I16 != SampleFormat::F32);
				REQUIRE(SampleFormat::I24 != SampleFormat::F32);
			}
		}
	}
}

SCENARIO("bytesPerSample falls back to 4 for an out-of-range enum value", "[sample-format]") {
	// The header has a defensive `return 4;` after an exhaustive switch.
	// Reach it by casting a value the enum doesn't define.
	GIVEN("an enum value outside the declared range") {
		const auto bogus = static_cast<SampleFormat>(99);
		THEN("the helper returns the F32 width as a safe default") {
			REQUIRE(bytesPerSample(bogus) == 4);
		}
	}
}

SCENARIO("SampleFormat fits in a single byte", "[sample-format]") {
	// The enum is declared `: uint8_t` because SampleData stores it inline.
	// If someone changes the underlying type the storage footprint regresses.
	THEN("sizeof(SampleFormat) == 1") {
		REQUIRE(sizeof(SampleFormat) == 1);
	}
}
