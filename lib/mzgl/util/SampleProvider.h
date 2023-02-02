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

class SampleProvider {
public:
	virtual ~SampleProvider() {}
	virtual const float operator[] (int index) const = 0;
	virtual size_t size() const = 0;
	virtual void splice(int start, int end, FloatBuffer &outBuff) const = 0;
	virtual bool isFloat() const = 0;
	virtual SampleProvider *shallowCopy() const = 0;
	virtual void getSamples(int startPos, FloatBuffer &buffToFill) const = 0;
	virtual void getSamples(int start, int length, float *outData) const = 0;
	
	// from to to-1 (e.g. end bounds exclusive)
	virtual void findMinMax(int from, int to, float &min, float &max) const = 0;
};


class FloatSampleProvider : public SampleProvider {
public:
	virtual ~FloatSampleProvider() {}
	FloatSampleProvider(const FloatBuffer &data) : data(data) {}
	
	const float operator[] (int index) const override {
		return data[index];
	}
	
	void getSamples(int startPos, FloatBuffer &buffToFill) const override {
		memcpy(buffToFill.data(), &data[startPos], buffToFill.size() * sizeof(float));
//		sample.getSamples(start*numChannels + inWinStart, framed);
	}

	void getSamples(int start, int length, float *outData) const override {
		memcpy(outData, &data[start], length * sizeof(float));
	}
	
	size_t size() const override {
		return data.size();
	}
	
	void splice(int start, int end, FloatBuffer &outBuff) const override {
		outBuff.clear();
		outBuff.insert(outBuff.end(), data.begin() + start, data.begin() + end);
	}
	
	void findMinMax(int from, int to, float &min, float &max) const override {
		float _min = 1;
		float _max = -1;
		for(int j = from; j < to; j++) {
			float v = data[j];
			if(_max<v) _max = v;
			if(_min>v) _min = v;
		}
		min = _min;
		max = _max;
	}

	bool isFloat() const override {
		return true;
	}
//	const std::vector<float> container() { return data; }
	virtual SampleProvider *shallowCopy() const override { return new FloatSampleProvider(data); };
private:
	const FloatBuffer &data;
};

class Int16SampleProvider : public SampleProvider {
public:
	virtual ~Int16SampleProvider() {}
	float mult;
	Int16SampleProvider(const std::vector<int16_t> &data) : data(data) {
		mult = 1.f / 32767.f;
	}
	size_t size() const override {
		return data.size();
	}
	const float operator[] (int index) const override {
		return data[index] * mult;
	}
	
	void findMinMax(int from, int to, float &_min, float &_max) const override {
		int16_t minVal = 32767;
		int16_t maxVal = -32768;
		for(int j = (int)from; j < to; j++) {
			const auto v = data[j];
			if(v>maxVal) maxVal = v;
			if(v<minVal) minVal = v;
		}
		_min = minVal * mult;
		_max = maxVal * mult;
	}
	
	void getSamples(int startPos, FloatBuffer &buffToFill) const override {
		for(int i = 0; i < buffToFill.size(); i++) {
			buffToFill[i] = (*this)[i+startPos];
		}
	}
	
	void getSamples(int start, int length, float *outData) const override {
		for(int i = 0; i < length; i++) {
			outData[i] = (*this)[i+start];
		}
	}
	
	void splice(int start, int end, FloatBuffer &outBuff) const override {
		outBuff.resize(outBuff.size() + end - start);
		for(int i = start; i < end; i++) {
			outBuff[i-start] = data[i] / 32767.f;
		}
	}
	bool isFloat() const override {
		return false;
	}
//	const std::vector<int16_t> container() { return data; }
	virtual SampleProvider *shallowCopy() const override { return new Int16SampleProvider(data); };
private:
	const std::vector<int16_t> &data;
};
