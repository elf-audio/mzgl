//
//  SampleReaders.h
//  mzgl
//
//  Integer-position typed readers + writers over a SampleData. Resolved
//  once per cold-path operation (makeMono, normalize, find non-silent,
//  nearest zero crossing, etc.) and called per sample/frame — so the
//  format dispatch cost is one indirect call per sample, not a runtime
//  switch. Distinct from voice_readers which take a `double` playhead
//  and do interpolation; these read at exact int positions.
//

#pragma once

#include "SampleData.h"
#include "SampleFormat.h"
#include <algorithm>
#include <cstdint>

namespace sample_readers {

	// One sample as float at sample-index `i` (already accounts for
	// channel interleave; frame `f` channel `c` is `i = f * numChans + c`).
	using SampleReadFn = float (*)(const SampleData &, int);

	// One frame's L+R as float at frame-index `f`. For mono samples R=L.
	using FrameReadFn = void (*)(const SampleData &, int, int, float &, float &);

	// Write float `v` into sample-index `i`, encoding per the buffer's format.
	using SampleWriteFn = void (*)(SampleData &, int, float);

	// ---- per-format scalar reads ----

	inline float read_i8(const SampleData &s, int i) noexcept {
		return s.asI8()[i] * (1.f / 128.f);
	}
	inline float read_i16(const SampleData &s, int i) noexcept {
		return s.asI16()[i] * (1.f / 32768.f);
	}
	inline float read_i24(const SampleData &s, int i) noexcept {
		const uint8_t *p = s.asI24() + i * 3;
		int32_t v		 = (int32_t(int8_t(p[2])) << 16) | (int32_t(p[1]) << 8) | int32_t(p[0]);
		return v * (1.f / 8388608.f);
	}
	inline float read_f32(const SampleData &s, int i) noexcept { return s.asF32()[i]; }

	// ---- per-format scalar writes ----

	inline void write_i8(SampleData &s, int i, float v) noexcept {
		s.asI8()[i] = static_cast<int8_t>(std::clamp(v, -1.f, 1.f) * 127.f);
	}
	inline void write_i16(SampleData &s, int i, float v) noexcept {
		s.asI16()[i] = static_cast<int16_t>(std::clamp(v, -1.f, 1.f) * 32767.f);
	}
	inline void write_i24(SampleData &s, int i, float v) noexcept {
		int32_t x	= static_cast<int32_t>(std::clamp(v, -1.f, 1.f) * 8388607.f);
		uint8_t *p	= s.asI24() + i * 3;
		p[0]		= static_cast<uint8_t>(x & 0xFF);
		p[1]		= static_cast<uint8_t>((x >> 8) & 0xFF);
		p[2]		= static_cast<uint8_t>((x >> 16) & 0xFF);
	}
	inline void write_f32(SampleData &s, int i, float v) noexcept { s.asF32()[i] = v; }

	// ---- frame-read helper bound to a per-format scalar reader ----

	template <SampleReadFn Read>
	void readFrame(const SampleData &s, int frameIdx, int numChans, float &L, float &R) noexcept {
		const int base = frameIdx * numChans;
		L			   = Read(s, base);
		R			   = numChans > 1 ? Read(s, base + 1) : L;
	}

	// ---- resolvers ----

	inline SampleReadFn resolveReadSample(SampleFormat fmt) noexcept {
		switch (fmt) {
			case SampleFormat::I8: return &read_i8;
			case SampleFormat::I16: return &read_i16;
			case SampleFormat::I24: return &read_i24;
			case SampleFormat::F32: return &read_f32;
		}
		return &read_f32;
	}

	inline SampleWriteFn resolveWriteSample(SampleFormat fmt) noexcept {
		switch (fmt) {
			case SampleFormat::I8: return &write_i8;
			case SampleFormat::I16: return &write_i16;
			case SampleFormat::I24: return &write_i24;
			case SampleFormat::F32: return &write_f32;
		}
		return &write_f32;
	}

	inline FrameReadFn resolveReadFrame(SampleFormat fmt) noexcept {
		switch (fmt) {
			case SampleFormat::I8: return &readFrame<read_i8>;
			case SampleFormat::I16: return &readFrame<read_i16>;
			case SampleFormat::I24: return &readFrame<read_i24>;
			case SampleFormat::F32: return &readFrame<read_f32>;
		}
		return &readFrame<read_f32>;
	}

} // namespace sample_readers
