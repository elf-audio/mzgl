
#pragma once
#include "FloatBuffer.h"

class Int16Buffer {
public:
	inline const float operator[](int index) const { return d[index] / 32767.f; }

	inline size_t size() const { return d.size(); }

	void reserve(const size_t count) { d.reserve(count); }

	void clear() { d.clear(); }

	void fadeIn(int length, int numChans, bool smooth = false);

	void fadeOut(int length, int numChans, bool smooth = false);

	void getMinMax(float &inMin, float &inMax) const;

	void normalizeAudio();

	float interpolate(double p) const noexcept;

	void interpolateStereo(double p, float &L, float &R) const noexcept;
	void assignValue(int index, float v) { d[index] = clamp16bit(v); }

	void set(const float *_d, size_t l) {
		d.resize(l);
		for (size_t i = 0; i < l; i++) {
			d[i] = clamp16bit(_d[i]);
		}
	}

	[[deprecated]] void splice(int start, int end, Int16Buffer &outBuff) const {
		outBuff.clear();
		outBuff.d.insert(outBuff.d.end(), d.begin() + start, d.begin() + end);
	}

	Int16Buffer splice(int start, int end) const {
		Int16Buffer outBuff;
		outBuff.d.insert(outBuff.d.end(), d.begin() + start, d.begin() + end);
		return outBuff;
	}

	void append(const Int16Buffer &b) { d.insert(d.end(), b.d.begin(), b.d.end()); }

	// mix incoming sample into this one
	void mix(const Int16Buffer &other);

	inline int16_t clamp16bit(float f) noexcept {
		return static_cast<int16_t>(std::clamp(f, -1.f, 1.f) * 32767.f);
	}
	void append(const FloatBuffer &fd);

	void append(const float *data, int length);
	size_t capacity() { return d.capacity(); }
	void resize(size_t size, float defaultValue = 0.f) { d.resize(size, defaultValue); }

	std::vector<int16_t> d;
	const int16_t *data() { return d.data(); }
};
