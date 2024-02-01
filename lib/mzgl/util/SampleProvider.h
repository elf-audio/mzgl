//
//  SampleProvider.h
//  mzgl
//
//  Created by Marek Bereza on 23/08/2021.
//  Copyright © 2021 Marek Bereza. All rights reserved.
//

// always gives you floats
#pragma once

#include <cstring>

#ifdef __ARM_NEON
#	include <arm_neon.h>
#endif

class SampleProvider {
public:
	virtual ~SampleProvider() {}
	virtual const float operator[](int index) const						 = 0;
	virtual size_t size() const											 = 0;
	virtual void splice(int start, int end, FloatBuffer &outBuff) const	 = 0;
	virtual bool isFloat() const										 = 0;
	virtual SampleProvider *shallowCopy() const							 = 0;
	virtual void getSamples(int startPos, FloatBuffer &buffToFill) const = 0;
	virtual void getSamples(int start, int length, float *outData) const = 0;

	// from to to-1 (e.g. end bounds exclusive)
	virtual void findMinMax(int from, int to, float &min, float &max) const = 0;

	[[nodiscard]] bool operator==(const SampleProvider &other) const {
		if (this == &other) return true;
		if (size() != other.size()) return false;

		for (size_t index = 0; index < size(); ++index) {
			if (std::abs((*this)[index] - other[index]) > 1e-4) {
				return false;
			}
		}

		return true;
	}
};

class FloatSampleProvider : public SampleProvider {
public:
	~FloatSampleProvider() override = default;
	FloatSampleProvider(const FloatBuffer &data)
		: data(data) {}

	const float operator[](int index) const override { return data[index]; }

	void getSamples(int startPos, FloatBuffer &buffToFill) const override {
		memcpy(buffToFill.data(), &data[startPos], buffToFill.size() * sizeof(float));
		//		sample.getSamples(start*numChannels + inWinStart, framed);
	}

	void getSamples(int start, int length, float *outData) const override {
		memcpy(outData, &data[start], length * sizeof(float));
	}

	size_t size() const override { return data.size(); }

	void splice(int start, int end, FloatBuffer &outBuff) const override {
		outBuff.clear();
		outBuff.insert(outBuff.end(), data.begin() + start, data.begin() + end);
	}

	void findMinMax(int from, int to, float &min, float &max) const override {
		// find min and max in float vector data between from and to
		// and set min and max to those values

		// intial benchmark shows that the std::minmax_element is much slower than
		// the algo below it
//#if 0
#ifdef __ARM_NEON
		int numIterations = (to - from) / 4; // Each iteration processes 4 values

		float32x4_t minVec = vdupq_n_f32(1);
		float32x4_t maxVec = vdupq_n_f32(-1);

		for (int i = 0; i < numIterations; i++) {
			float32x4_t values = vld1q_f32(this->data.data() + from + i * 4);
			minVec			   = vminq_f32(minVec, values);
			maxVec			   = vmaxq_f32(maxVec, values);
		}

		auto _min = vminvq_f32(minVec);
		auto _max = vmaxvq_f32(maxVec);

		for (int j = from + numIterations * 4; j < to; j++) {
			float v = this->data[j];
			if (_max < v) _max = v;
			if (_min > v) _min = v;
		}
		min = _min;
		max = _max;

#else
		// Fallback for platforms without NEON support
		float _min = 1;
		float _max = -1;
		for (int j = from; j < to; j++) {
			float v = this->data[j];
			if (_max < v) _max = v;
			if (_min > v) _min = v;
		}
		min = _min;
		max = _max;
#endif
	}

	bool isFloat() const override { return true; }
	//	const std::vector<float> container() { return data; }
	virtual SampleProvider *shallowCopy() const override { return new FloatSampleProvider(data); };

private:
	const FloatBuffer &data;
};

class Int16SampleProvider : public SampleProvider {
public:
	virtual ~Int16SampleProvider() {}
	float mult;
	Int16SampleProvider(const std::vector<int16_t> &data)
		: data(data) {
		mult = 1.f / 32767.f;
	}
	size_t size() const override { return data.size(); }
	const float operator[](int index) const override { return data[index] * mult; }

	void findMinMax(int from, int to, float &_min, float &_max) const override {
#ifdef __ARM_NEON
		int numIterations = (to - from) / 8; // Each iteration processes 8 values

		int16x8_t minVec = vdupq_n_s16(32767);
		int16x8_t maxVec = vdupq_n_s16(-32768);

		for (int i = 0; i < numIterations; i++) {
			int16x8_t values = vld1q_s16(&data[from + i * 8]);
			minVec			 = vminq_s16(minVec, values);
			maxVec			 = vmaxq_s16(maxVec, values);
		}

		int16_t resultsMin[8], resultsMax[8];
		vst1q_s16(resultsMin, minVec);
		vst1q_s16(resultsMax, maxVec);

		int16_t minVal = *std::min_element(resultsMin, resultsMin + 8);
		int16_t maxVal = *std::max_element(resultsMax, resultsMax + 8);

		// Handle any remaining values that aren't a multiple of 8
		for (int j = from + numIterations * 8; j < to; j++) {
			const auto v = data[j];
			if (v > maxVal) maxVal = v;
			if (v < minVal) minVal = v;
		}
		_min = minVal * mult;
		_max = maxVal * mult;

#else
		// Fallback for platforms without NEON support
		int16_t minVal = 32767;
		int16_t maxVal = -32768;
		for (int j = from; j < to; j++) {
			const auto v = data[j];
			if (v > maxVal) maxVal = v;
			if (v < minVal) minVal = v;
		}
		_min = minVal * mult;
		_max = maxVal * mult;
#endif
	}

	void getSamples(int startPos, FloatBuffer &buffToFill) const override {
		for (int i = 0; i < buffToFill.size(); i++) {
			buffToFill[i] = (*this)[i + startPos];
		}
	}

	void getSamples(int start, int length, float *outData) const override {
		for (int i = 0; i < length; i++) {
			outData[i] = (*this)[i + start];
		}
	}

	void splice(int start, int end, FloatBuffer &outBuff) const override {
		outBuff.resize(outBuff.size() + end - start);
		for (int i = start; i < end; i++) {
			outBuff[i - start] = data[i] / 32767.f;
		}
	}
	bool isFloat() const override { return false; }
	//	const std::vector<int16_t> container() { return data; }
	virtual SampleProvider *shallowCopy() const override { return new Int16SampleProvider(data); };

private:
	const std::vector<int16_t> &data;
};
