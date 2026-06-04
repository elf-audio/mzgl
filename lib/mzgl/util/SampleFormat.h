//
//  SampleFormat.h
//  mzgl
//
//  Tag for audio buffers stored in their native bit depth. Used by
//  AudioFile::loadNative to communicate the on-disk format and by
//  koala's SampleData wrapper to dispatch reads/writes per format.
//

#pragma once

#include <cstdint>
#include <cstring>

enum class SampleFormat : uint8_t {
	I8,	 // signed 8-bit, 1 byte/sample
	I16, // signed 16-bit, 2 bytes/sample
	I24, // signed 24-bit packed little-endian, 3 bytes/sample
	F32, // 32-bit float, 4 bytes/sample
};

inline int bytesPerSample(SampleFormat f) noexcept {
	switch (f) {
		case SampleFormat::I8: return 1;
		case SampleFormat::I16: return 2;
		case SampleFormat::I24: return 3;
		case SampleFormat::F32: return 4;
	}
	return 4;
}

// Decode 24-bit signed packed-LE to a float in [-1, +1).
//
// Reads sizeof(int32_t) = 4 bytes — so this is safe to call only where the
// caller can guarantee at least one byte of readable storage past `p + 2`.
// `SampleData` provides 4 trailing pad bytes for exactly this purpose
// (`SampleData::kTailPad`), so any reader that goes through a SampleData
// can use this. memcpy is the strict-aliasing-safe way to express an
// unaligned load; on Apple Silicon and modern x86 the compiler lowers it
// to a single load instruction.
//
// Mathematically identical to the explicit shift-based form
//     (int32_t(int8_t(p[2])) << 16) | (int32_t(p[1]) << 8) | int32_t(p[0])
// but typically 30–50% faster on Arm64 because it's one load instead of three
// plus the byte shuffles. Asserted bit-identical in sampleFormatTests.cpp.
inline float decodeI24(const uint8_t *p) noexcept {
	int32_t v;
	std::memcpy(&v, p, sizeof(v));
	v = (v << 8) >> 8; // sign-extend from bit 23
	return v * (1.0f / 8388608.0f);
}
