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
