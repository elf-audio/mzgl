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

// =============================================================================
// decodeI24 — proves that the optimised unaligned-load + sign-extend
// implementation produces bit-identical output to the explicit shift form
// it replaced. SuperSampleReaders.h / SampleReaders.h / SampleData::operator[]
// all call decodeI24 on the hot path, so this is the equivalence net.
// =============================================================================

namespace {
	// The shift-based implementation as it existed before optimisation. Reads
	// three bytes one at a time, sign-extends the high byte, ORs the parts
	// together, then divides by 2^23 for the float scale.
	inline float decodeI24_old(const uint8_t *p) noexcept {
		const int32_t v =
			(int32_t(int8_t(p[2])) << 16) | (int32_t(p[1]) << 8) | int32_t(p[0]);
		return v * (1.0f / 8388608.0f);
	}

	// Pack a signed-24 value into three little-endian bytes. Mirrors what
	// SampleData::assignValue and DrAudioFileReader emit on disk.
	void packI24(int32_t value, uint8_t out[3]) noexcept {
		out[0] = static_cast<uint8_t>(value & 0xFF);
		out[1] = static_cast<uint8_t>((value >> 8) & 0xFF);
		out[2] = static_cast<uint8_t>((value >> 16) & 0xFF);
	}
} // namespace

TEST_CASE("decodeI24 matches the original shift-based form at the int24 boundaries",
		  "[sample-format][decode-i24]") {
	// Five trailing zero bytes so decodeI24's unaligned 4-byte load is safe
	// at every offset we test.
	std::array<uint8_t, 8> buf {};

	const std::vector<int32_t> values = {
		0,
		1,
		-1,
		127,
		-128,
		32767,
		-32768,
		8388607,	// max int24
		-8388608, // min int24
		8388606,
		-8388607,
		8388607 - 1,
		-8388608 + 1,
		// Random mid-range
		1234567,
		-7654321,
		0x7FFFFF & 0xAAAAAA, // alternating-bit canary
		~0x7FFFFF & 0x555555,
	};

	for (auto v : values) {
		packI24(v, buf.data());
		const float decoded_new = decodeI24(buf.data());
		const float decoded_old = decodeI24_old(buf.data());
		INFO("v = " << v);
		REQUIRE(decoded_new == decoded_old);
	}
}

TEST_CASE("decodeI24 is bit-identical across a comprehensive int24 sweep",
		  "[sample-format][decode-i24]") {
	// Sweep every 257th value across the full int24 range. 257 is coprime
	// with 2^k so it hits every sign-bit pattern. That's 65,278 distinct
	// values — fast (a few ms) but exhaustive enough to catch any error in
	// the sign-extend boundary.
	std::array<uint8_t, 8> buf {};

	int mismatches = 0;
	int32_t first_bad = 0;
	for (int32_t v = -8388608; v <= 8388607; v += 257) {
		packI24(v, buf.data());
		const float a = decodeI24(buf.data());
		const float b = decodeI24_old(buf.data());
		if (a != b) {
			if (mismatches == 0) first_bad = v;
			++mismatches;
		}
	}
	INFO("first mismatch at v = " << first_bad);
	REQUIRE(mismatches == 0);
}

TEST_CASE("decodeI24 sweeps all sign-bit transitions exactly",
		  "[sample-format][decode-i24]") {
	// Walk every value in [-1024, +1024] — covers both the small-positive
	// and small-negative regions plus the sign-bit boundary at 0.
	std::array<uint8_t, 8> buf {};
	for (int32_t v = -1024; v <= 1024; ++v) {
		packI24(v, buf.data());
		const float a = decodeI24(buf.data());
		const float b = decodeI24_old(buf.data());
		INFO("v = " << v);
		REQUIRE(a == b);
	}
}

TEST_CASE("decodeI24 sweeps the int24 negative boundary",
		  "[sample-format][decode-i24]") {
	// The most likely failure mode of the sign-extend trick is "almost-zero
	// negative" or "near-int24-min" values where the upper byte's MSB
	// matters. Hit those exhaustively.
	std::array<uint8_t, 8> buf {};
	for (int32_t v = -8388608; v < -8388608 + 4096; ++v) {
		packI24(v, buf.data());
		REQUIRE(decodeI24(buf.data()) == decodeI24_old(buf.data()));
	}
	for (int32_t v = 8388607; v > 8388607 - 4096; --v) {
		packI24(v, buf.data());
		REQUIRE(decodeI24(buf.data()) == decodeI24_old(buf.data()));
	}
}
