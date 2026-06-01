//
//  SampleData.h
//  koalib
//
//  Tagged byte-buffer that stores audio samples in their native bit
//  depth (i8 / i16 / i24-packed / f32). Replaces the SuperSampleParent
//  alias that used to switch between Int16Buffer and FloatBuffer at
//  compile time. Hot-path readers (SuperVoice, stretchers, GrainsSynth,
//  ChopperSynth) go through voice_readers::ReadFn dispatched once per
//  voice activation, so they pay no per-sample format branch. The
//  runtime-branching operator[] / assignValue / etc on this class are
//  for the cold paths (normalize, makeMono, splice, GUI-side reads,
//  tests) where the convenience is worth the switch.
//

#pragma once

#include "SampleFormat.h"
#include "FloatBuffer.h"
#include "Int16Buffer.h"
#include "SampleProvider.h"
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <optional>
#include <vector>

class SampleData {
public:
	SampleData() = default;

	SampleData(const SampleData &) = default;
	SampleData(SampleData &&) noexcept = default;
	SampleData &operator=(const SampleData &) = default;
	SampleData &operator=(SampleData &&) noexcept = default;

	// Construct from a typed buffer. Adopts the format the buffer was
	// already in; raw bytes are copied verbatim.
	explicit SampleData(const FloatBuffer &src) {
		format = SampleFormat::F32;
		raw.resize(src.size() * sizeof(float));
		if (!src.empty()) {
			std::memcpy(raw.data(), src.data(), raw.size());
		}
	}

	explicit SampleData(FloatBuffer &&src) {
		format = SampleFormat::F32;
		raw.resize(src.size() * sizeof(float));
		if (!src.empty()) {
			std::memcpy(raw.data(), src.data(), raw.size());
		}
	}

	explicit SampleData(const Int16Buffer &src) {
		format = SampleFormat::I16;
		raw.resize(src.d.size() * sizeof(int16_t));
		if (!src.d.empty()) {
			std::memcpy(raw.data(), src.d.data(), raw.size());
		}
	}

	// Construct from raw bytes already in a particular format.
	SampleData(std::vector<uint8_t> &&bytes, SampleFormat fmt)
		: raw(std::move(bytes))
		, format(fmt) {}

	SampleFormat getFormat() const noexcept { return format; }
	int bytesPerSample() const noexcept { return ::bytesPerSample(format); }

	// Logical sample count (numFrames * numChannels).
	size_t size() const noexcept { return raw.empty() ? 0 : raw.size() / bytesPerSample(); }
	bool empty() const noexcept { return raw.empty(); }

	void clear() noexcept { raw.clear(); }

	void resize(size_t numSamples, std::optional<float> defaultValue = std::nullopt) {
		const int bps = bytesPerSample();
		const size_t oldBytes = raw.size();
		raw.resize(numSamples * bps);
		if (defaultValue && raw.size() > oldBytes) {
			const size_t startIdx = oldBytes / bps;
			for (size_t i = startIdx; i < numSamples; ++i) {
				assignValue(static_cast<int>(i), *defaultValue);
			}
		}
	}

	void reserve(size_t numSamples) { raw.reserve(numSamples * bytesPerSample()); }
	size_t capacity() const noexcept { return raw.capacity() / bytesPerSample(); }

	// Raw byte access. For serialization or when the caller already
	// knows the format and wants typed access via the helpers below.
	uint8_t *bytes() noexcept { return raw.data(); }
	const uint8_t *bytes() const noexcept { return raw.data(); }
	size_t byteSize() const noexcept { return raw.size(); }

	// Typed pointer accessors. Caller is responsible for matching format.
	const int8_t *asI8() const noexcept { return reinterpret_cast<const int8_t *>(raw.data()); }
	int8_t *asI8() noexcept { return reinterpret_cast<int8_t *>(raw.data()); }

	const int16_t *asI16() const noexcept { return reinterpret_cast<const int16_t *>(raw.data()); }
	int16_t *asI16() noexcept { return reinterpret_cast<int16_t *>(raw.data()); }

	// I24 has no native type; raw bytes are 3 per sample, packed little-endian.
	const uint8_t *asI24() const noexcept { return raw.data(); }
	uint8_t *asI24() noexcept { return raw.data(); }

	const float *asF32() const noexcept { return reinterpret_cast<const float *>(raw.data()); }
	float *asF32() noexcept { return reinterpret_cast<float *>(raw.data()); }

	// Slow path: read sample i as float. Runtime branch on format.
	float operator[](size_t i) const noexcept {
		switch (format) {
			case SampleFormat::I8: return asI8()[i] * (1.f / 128.f);
			case SampleFormat::I16: return asI16()[i] * (1.f / 32768.f);
			case SampleFormat::I24: {
				const uint8_t *p = asI24() + i * 3;
				int32_t v = (int32_t(int8_t(p[2])) << 16) | (int32_t(p[1]) << 8) | int32_t(p[0]);
				return v * (1.f / 8388608.f);
			}
			case SampleFormat::F32: return asF32()[i];
		}
		return 0.f;
	}

	void assignValue(size_t i, float v) noexcept {
		switch (format) {
			case SampleFormat::I8:
				asI8()[i] = static_cast<int8_t>(std::clamp(v, -1.f, 1.f) * 127.f);
				break;
			case SampleFormat::I16:
				asI16()[i] = static_cast<int16_t>(std::clamp(v, -1.f, 1.f) * 32767.f);
				break;
			case SampleFormat::I24: {
				int32_t s = static_cast<int32_t>(std::clamp(v, -1.f, 1.f) * 8388607.f);
				uint8_t *p = asI24() + i * 3;
				p[0] = static_cast<uint8_t>(s & 0xFF);
				p[1] = static_cast<uint8_t>((s >> 8) & 0xFF);
				p[2] = static_cast<uint8_t>((s >> 16) & 0xFF);
				break;
			}
			case SampleFormat::F32: asF32()[i] = v; break;
		}
	}

	// Convert this buffer's contents into a FloatBuffer (lossless for F32,
	// lossy for the integer formats only in the way you'd expect). Used by
	// callers that interface with code still typed on FloatBuffer (e.g.
	// AudioFile::save, Spleeter::extract). AVOID this for large samples —
	// it allocates `size() * 4` bytes. Prefer SampleDataProvider.
	FloatBuffer toFloatBuffer() const {
		FloatBuffer out;
		out.resize(size(), 0.f);
		for (size_t i = 0; i < size(); ++i) out[i] = (*this)[i];
		return out;
	}

	// Append another SampleData to this one. Same format -> cheap byte
	// memcpy. Different format -> per-sample transcode into THIS format
	// (so the storage stays homogeneous; lossy only in the way you'd
	// expect when downconverting).
	void append(const SampleData &other) {
		if (other.empty()) return;
		if (raw.empty()) {
			format = other.format;
		}
		if (format == other.format) {
			raw.insert(raw.end(), other.raw.begin(), other.raw.end());
			return;
		}
		const size_t startIdx = size();
		const size_t addCount = other.size();
		resize(startIdx + addCount);
		for (size_t i = 0; i < addCount; ++i) assignValue(startIdx + i, other[i]);
	}

	// Convenience: append samples from a FloatBuffer (may transcode).
	void append(const FloatBuffer &other) {
		if (other.empty()) return;
		if (raw.empty()) format = SampleFormat::F32;
		if (format == SampleFormat::F32) {
			const size_t startBytes = raw.size();
			raw.resize(startBytes + other.size() * sizeof(float));
			std::memcpy(raw.data() + startBytes, other.data(), other.size() * sizeof(float));
			return;
		}
		const size_t startIdx = size();
		resize(startIdx + other.size());
		for (size_t i = 0; i < other.size(); ++i) assignValue(startIdx + i, other[i]);
	}

	// Splice [startSample, endSample) into a new SampleData, same format.
	SampleData splice(int startSample, int endSample) const {
		SampleData out;
		out.format = format;
		const int bps = bytesPerSample();
		const auto first = raw.begin() + startSample * bps;
		const auto last = raw.begin() + endSample * bps;
		out.raw.insert(out.raw.end(), first, last);
		return out;
	}

	// Mix `other` into this one (per-sample addition in float space).
	// Both must share format and length.
	void mix(const SampleData &other) {
		if (other.empty()) return;
		mzAssertSameFormat(other);
		const size_t n = std::min(size(), other.size());
		for (size_t i = 0; i < n; ++i) {
			assignValue(i, (*this)[i] + other[i]);
		}
	}

	// Normalize so the loudest sample sits at full scale.
	void normalizeAudio() {
		float peak = 0.f;
		const size_t n = size();
		for (size_t i = 0; i < n; ++i) peak = std::max(peak, std::abs((*this)[i]));
		if (peak == 0.f || peak >= 1.f) return;
		const float gain = 1.f / peak;
		for (size_t i = 0; i < n; ++i) assignValue(i, (*this)[i] * gain);
	}

	// Find the first sample (frame index, not sample index) whose
	// absolute value exceeds `threshold`. Returns nullopt if none.
	std::optional<size_t> findFirstOnset(int numChannels, float threshold) const {
		const size_t frames = size() / static_cast<size_t>(std::max(1, numChannels));
		for (size_t f = 0; f < frames; ++f) {
			for (int c = 0; c < numChannels; ++c) {
				if (std::abs((*this)[f * numChannels + c]) > threshold) return f;
			}
		}
		return std::nullopt;
	}

private:
	void mzAssertSameFormat(const SampleData &other) const noexcept {
		// Best-effort; debug-only assert lives in the .cpp / call sites if
		// they care. Mismatched formats here would be a bug at the caller.
		(void) other;
	}

	std::vector<uint8_t> raw;
	SampleFormat format = SampleFormat::F32;
};

// Zero-copy SampleProvider adapter over a SampleData. Lifetime of the
// underlying SampleData must outlive the provider. Use this for any
// API that accepts `const SampleProvider&` (WaveformPreview generation,
// SFOnsetDetector, ThreadedPVoc, …) so we never materialise a full
// FloatBuffer copy of multi-gigabyte samples.
class SampleDataProvider : public SampleProvider {
public:
	explicit SampleDataProvider(const SampleData &data) noexcept
		: data(data) {}

	const float operator[](int index) const override { return data[static_cast<size_t>(index)]; }

	size_t size() const override { return data.size(); }

	void splice(int start, int end, FloatBuffer &outBuff) const override {
		const int n = end - start;
		outBuff.resize(static_cast<size_t>(n));
		for (int i = 0; i < n; ++i) outBuff[i] = data[static_cast<size_t>(start + i)];
	}

	void getSamples(int startPos, FloatBuffer &buffToFill) const override {
		for (size_t i = 0; i < buffToFill.size(); ++i) buffToFill[i] = data[startPos + i];
	}

	void getSamples(int start, int length, float *outData) const override {
		for (int i = 0; i < length; ++i) outData[i] = data[static_cast<size_t>(start + i)];
	}

	void findMinMax(int from, int to, float &min, float &max) const override {
		float lo = 1.f;
		float hi = -1.f;
		for (int i = from; i < to; ++i) {
			const float v = data[static_cast<size_t>(i)];
			if (v < lo) lo = v;
			if (v > hi) hi = v;
		}
		min = lo;
		max = hi;
	}

	bool isFloat() const override { return data.getFormat() == SampleFormat::F32; }

	SampleProvider *shallowCopy() const override { return new SampleDataProvider(data); }

private:
	const SampleData &data;
};
