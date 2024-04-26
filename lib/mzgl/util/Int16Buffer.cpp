
#include "Int16Buffer.h"
#include "maths.h"
void Int16Buffer::fadeIn(int length, int numChans, bool smooth) {
	if (size() * numChans > length) {
		if (numChans == 1) {
			for (int i = 0; i < length; i++) {
				float fade = i / (float) length;
				if (smooth) fade = smoothstep(fade);
				assignValue(i, (*this)[i] * fade);
			}
		} else if (numChans == 2) {
			for (int i = 0; i < length; i++) {
				float fade = i / (float) length;
				if (smooth) fade = smoothstep(fade);
				assignValue(i * 2, (*this)[i * 2] * fade);
				assignValue(i * 2 + 1, (*this)[i * 2 + 1] * fade);
			}
		} else {
			for (int i = 0; i < length; i++) {
				float fade = i / (float) length;
				if (smooth) fade = smoothstep(fade);
				for (int ch = 0; ch < numChans; ch++) {
					assignValue(i * numChans + ch, (*this)[i * numChans + ch] * fade);
				}
			}
		}
	} else {
		printf(
			"ERROR: Int16Buffer::fadeIn() - trying to fadeIn(%d, %d) on a sample that is only %lu samples long\n",
			length,
			numChans,
			static_cast<unsigned long>(size()));
	}
}

void Int16Buffer::fadeOut(int length, int numChans, bool smooth) {
	if (size() >= length * numChans) {
		if (numChans == 1) {
			for (int i = 0; i < length; i++) {
				float fade = i / (float) length;
				if (smooth) fade = smoothstep(fade);
				assignValue(static_cast<int>(size() - i - 1), (*this)[(int) size() - i - 1] * fade);
			}
		} else if (numChans == 2) {
			for (int i = 0; i < length; i++) {
				float fade = i / (float) length;
				if (smooth) fade = smoothstep(fade);
				int frameIndex = ((int) size() / numChans) - i - 1;
				assignValue(frameIndex * 2, (*this)[frameIndex * 2] * fade);
				assignValue(frameIndex * 2 + 1, (*this)[frameIndex * 2 + 1] * fade);
			}
		} else {
			for (int i = 0; i < length; i++) {
				float fade = i / (float) length;
				if (smooth) fade = smoothstep(fade);
				int frameIndex = ((int) size() / numChans) - i - 1;
				for (int ch = 0; ch < numChans; ch++) {
					assignValue(frameIndex * numChans + ch, (*this)[frameIndex * numChans + ch] * fade);
				}
			}
		}
	} else {
		printf(
			"ERROR: Int16Buffer::fadeOut() - trying to fadeOut(%d, %d) on a sample that is only %lu samples long\n",
			length,
			numChans,
			static_cast<unsigned long>(size()));
	}
}

void Int16Buffer::getMinMax(float &inMin, float &inMax) const {
	if (size() == 0) return;
	int16_t _inMin = 32767;
	int16_t _inMax = -32767;

	for (int i = 0; i < size(); i++) {
		if (d[i] < _inMin) {
			_inMin = d[i];
		}
		if (d[i] > _inMax) {
			_inMax = d[i];
		}
	}
	inMin = _inMin / 32767.f;
	inMax = _inMax / 32767.f;
}

void Int16Buffer::normalizeAudio() {
	float inMin, inMax;
	getMinMax(inMin, inMax);

	float loudest = std::max(std::abs(inMin), std::abs(inMax));
	if (loudest < 0.000001) {
		loudest = 0.00001;
	}
	float gain = 1.f / loudest;
	for (int i = 0; i < size(); i++) {
		assignValue(i, (*this)[i] * gain);
	}
}

std::optional<size_t> Int16Buffer::findFirstOnset(float threshold) const {
	if (empty()) {
		return std::nullopt;
	}

	size_t sampleCounter = 0;
	
	auto thresholdInt16 = static_cast<int16_t>(threshold * 32767.f);
	for (int i = 0; i < size(); i++) {
		if (std::abs((*this)[i]) > thresholdInt16) {
			return sampleCounter;
		}

		++sampleCounter;
	}

	return std::nullopt;
}


float Int16Buffer::interpolate(double p) const noexcept {
	int a	= p;
	int b	= a + 1;
	float m = p - a;
	if (b >= size()) {
		if (a >= size()) {
			a = b = (int) size() - 1;
		} else {
			b = a;
		}
	}
	return (d[a] * (1.f - m) + d[b] * m) / 32767.f;
}

void Int16Buffer::interpolateStereo(double p, float &L, float &R) const noexcept {
	int a	= p;
	int b	= a + 1;
	float m = p - a;
	if (b * 2 + 1 >= size()) {
		if (a * 2 + 1 >= size()) {
			a = b = (int) size() / 2 - 2;
		} else {
			b = a;
		}
	}

	L = (d[a * 2] * (1.f - m) + d[b * 2] * m) / 32767.f;
	R = (d[a * 2 + 1] * (1.f - m) + d[b * 2 + 1] * m) / 32767.f;
}

// mix incoming sample into this one
void Int16Buffer::mix(const Int16Buffer &other) {
	// make sure both samples are
	// as long as the longest one
	if (size() < other.size()) {
		resize(other.size(), 0.f);
	}

	// TODO: Accelerate
	for (int i = 0; i < other.size(); i++) {
		assignValue(i, (*this)[i] + other[i]);
	}
}

void Int16Buffer::append(const FloatBuffer &fd) {
	auto originalSize = d.size();
	d.resize(d.size() + fd.size());
	for (int i = 0; i < fd.size(); i++) {
		d[i + originalSize] = clamp16bit(fd[i]);
	}
}

void Int16Buffer::append(const float *data, int length) {
	auto originalSize = d.size();
	d.resize(d.size() + length);
	for (int i = 0; i < length; i++) {
		d[i + originalSize] = clamp16bit(data[i]);
	}
}
