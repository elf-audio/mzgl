
#include "FloatBuffer.h"
#include <assert.h>
//#include "util.h"
#include "maths.h"
#include <fstream>
#include <limits> // std::numeric_limits
#include <cmath> // std::isnan()
#include <string.h> // memset()
#ifdef __APPLE__
#	include <Accelerate/Accelerate.h>
#elif defined(__ARM_NEON)
#	include <arm_neon.h>
#endif

// xsimd disabled for MSVC build, it does not give performance boost for Android anyway - to be investigated
//#include "xsimd/xsimd.hpp"

using namespace std;

void FloatBuffer::linspace(float a, float b, int N) {
	float h = (b - a) / ((float) N - 1.f);
	resize(N);
	for (int i = 0; i < N; i++) {
		(*this)[i] = a + h * i;
	}
}

FloatBuffer::FloatBuffer(size_t sz) {
	zeros(sz);
}

FloatBuffer::~FloatBuffer() {
}

FloatBuffer::FloatBuffer(const float *arr, size_t size) {
	set(arr, size);
}

void FloatBuffer::splice(int start, int end, FloatBuffer &outBuff) const {
	outBuff.clear();
	outBuff.insert(outBuff.end(), begin() + start, begin() + end);
}

FloatBuffer FloatBuffer::splice(int start, int end) const {
	FloatBuffer outBuff;
	outBuff.insert(outBuff.end(), begin() + start, begin() + end);
	return outBuff;
}

void FloatBuffer::set(const float *buff, size_t length, int start, int stride) {
	if (start == 0 && stride == 1) {
		assign(buff, buff + length);
	} else {
		resize(length);
		int j = start;
		for (int i = 0; i < length; i++) {
			(*this)[i] = buff[j];
			j += stride;
		}
	}
}
void FloatBuffer::setMonoFromStereo(const float *buff, size_t numFrames) {
	resize(numFrames);
	for (int i = 0; i < numFrames; i++) {
		(*this)[i] = (buff[i * 2] + buff[i * 2 + 1]) * 0.5f;
	}
}
void FloatBuffer::setStereoFromMono(const float *data, int length) {
	resize(length * 2);
	for (int i = 0; i < length; i++) {
		(*this)[i * 2]	   = data[i];
		(*this)[i * 2 + 1] = data[i];
	}
}

int16_t FloatBuffer::floatToInt16(float in) const {
	return in * 32767.f;
}
vector<int16_t> FloatBuffer::getInt16() const {
	vector<int16_t> buff;
	buff.resize(size());
	for (int i = 0; i < size(); i++) {
		buff[i] = floatToInt16((*this)[i]);
	}
	return buff;
}

FloatBuffer FloatBuffer::stereoToMono() {
	FloatBuffer ret;
	ret.resize(size() / 2);
	for (int i = 0; i < ret.size(); i++) {
		ret[i] = ((*this)[i * 2] + (*this)[i * 2 + 1]) * 0.5;
	}
	return ret;
}

//void FloatBuffer::save(string path) {
//	ofstream myfile;
//	myfile.open (path.c_str());
//
//
//	for(int i = 0; i < size(); i++) {
//		if(i>0) myfile << ",";
//		myfile << to_string((*this)[i]);
//	}
//
//
//	myfile.close();
//}

//void FloatBuffer::load(string path) {
//	string line;
//	string f;
//	fs::ifstream myfile (fs::u8path(path.c_str()));
//	if (myfile.is_open())
//	{
//		while ( getline (myfile,line) )
//		{
//			f += line + "\n";
//		}
//		myfile.close();
//	}
//
//	vector<string> smps = split(f, ",");
//	resize(smps.size());
//	for(int i =0 ; i < smps.size(); i++) {
//		(*this)[i] = stof(smps[i]);
//	}
//}

float FloatBuffer::sum() const {
	float total = 0.f;
	for (int i = 0; i < size(); i++) {
		total += (*this)[i];
	}
	return total;
}
void FloatBuffer::square() {
	for (int i = 0; i < size(); i++) {
		(*this)[i] *= (*this)[i];
	}
}
float FloatBuffer::mean() const {
	return sum() / (float) size();
}

float FloatBuffer::absMax() const {
	return abs((*this)[findAbsMaxPos()]);
}

float FloatBuffer::maxValue() const {
#ifdef __APPLE__
	float maxVal = -INFINITY;
	vDSP_Length pos;
	vDSP_maxvi(data(), 1, &maxVal, &pos, size());
	return maxVal;
#else

	return (*this)[findMaxPos()];
#endif
}

float FloatBuffer::minValue() const {
#ifdef __APPLE__
	float minVal = INFINITY;
	vDSP_Length pos;
	vDSP_maxvi(data(), 1, &minVal, &pos, size());
	return minVal;
#else
	return (*this)[findMinPos()];
#endif
}

int FloatBuffer::findAbsMaxPos() const {
	float max = 0;
	int pos	  = 0;
	for (int i = 0; i < size(); i++) {
		if (abs((*this)[i]) > max) {
			max = abs((*this)[i]);
			pos = i;
		}
	}
	return pos;
}

int FloatBuffer::findMaxPos() const {
	float maxVal = std::numeric_limits<float>::min();
#ifdef __APPLE__
	vDSP_Length pos;
	vDSP_maxvi(data(), 1, &maxVal, &pos, size());
#else
	int pos = 0;
	for (int i = 0; i < size(); i++) {
		if ((*this)[i] > maxVal) {
			maxVal = (*this)[i];
			pos	   = i;
		}
	}
#endif
	return pos;
}

int FloatBuffer::findMinPos() const {
	float minVal = std::numeric_limits<float>::max();
#ifdef __APPLE__
	vDSP_Length pos;
	vDSP_maxvi(data(), 1, &minVal, &pos, size());
	return minVal;
#else
	int pos = 0;
	for (int i = 0; i < size(); i++) {
		if ((*this)[i] < minVal) {
			minVal = (*this)[i];
			pos	   = i;
		}
	}
#endif
	return pos;
}

void FloatBuffer::getMinMax(float &min, float &max) const {
	if (size() == 0) return;
	min = std::numeric_limits<float>::max();
	max = std::numeric_limits<float>::min();

	for (int i = 0; i < size(); i++) {
		if ((*this)[i] < min) {
			min = (*this)[i];
		}
		if ((*this)[i] > max) {
			max = (*this)[i];
		}
	}
}

void FloatBuffer::normalize(float min, float max) {
	float inMin, inMax;
	getMinMax(inMin, inMax);

	float norm	= 1.f / (inMax - inMin);
	float scale = max - min;

	for (int i = 0; i < size(); i++) {
		(*this)[i] = min + scale * ((*this)[i] - inMin) * norm;
	}
}

void FloatBuffer::normalizeAudio() {
	float inMin, inMax;
	getMinMax(inMin, inMax);

	float loudest = std::max(abs(inMin), abs(inMax));
	if (loudest < 0.000001) {
		loudest = 0.00001;
	}
	float gain = 1.f / loudest;
	(*this) *= gain;
}

void FloatBuffer::append(float *buff, int length) {
	insert(end(), buff, buff + length);
}

void FloatBuffer::append(const FloatBuffer &b) {
	insert(end(), b.begin(), b.end());
}

void FloatBuffer::setFromLeftChannel(float *buff, int length) {
	set(buff, length, 0, 2);
}

void FloatBuffer::setFromRightChannel(float *buff, int length) {
	set(buff, length, 1, 2);
}

// mix incoming sample into this one
void FloatBuffer::mix(const FloatBuffer &other) {
	// make this sample is at least as long as the other
	if (size() < other.size()) {
		resize(other.size(), 0.f);
	}
// mix the other in - note that we're going for a length of other.size()
// - other might be shorter, but it mustn't be longer than this.
#ifdef __APPLE__
	vDSP_vadd(other.data(), 1, data(), 1, data(), 1, other.size());
#else

	// needs -ffast-math - maybe the android build will benefit from this?
	// you can apply it per file if you don't want it everywhere
	// for some reason this is still slow on android - can't seem to get xsimd working
	//	auto sz = size();
	//	constexpr auto simd_size = xsimd::simd_type<float>::size;
	//	auto vec_size = sz - sz % simd_size;
	//
	//	for(std::size_t i = 0; i < vec_size; i += simd_size) {
	//		auto ba = xsimd::load_aligned(&(*this)[i]);
	//		auto bb = xsimd::load_aligned(&other[i]);
	//		auto bres = (ba + bb);
	//		bres.store_aligned(&(*this)[i]);
	//	}
	//
	//	for(std::size_t i = vec_size; i < sz; ++i) {
	//		(*this)[i] += other[i];
	//	}

	for (int i = 0; i < other.size(); i++) {
		(*this)[i] += other[i];
	}
#endif
}

// mix incoming sample into this one
void FloatBuffer::mixNaive(const FloatBuffer &other) {
	// make sure both samples are
	// as long as the longest one
	if (size() < other.size()) {
		resize(other.size(), 0.f);
	}

	for (int i = 0; i < other.size(); i++) {
		(*this)[i] += other[i];
	}
}

void FloatBuffer::random(float min, float max) {
	for (int i = 0; i < size(); i++) {
		(*this)[i] = randf(min, max);
	}
}

// sets the input samples to the contents of this FloatBuffer
void FloatBuffer::get(float *buff) const {
	memcpy(buff, data(), size() * sizeof(float));
}

void FloatBuffer::get(float *buff, int len) const {
	memcpy(buff, data(), len * sizeof(float));
}

void FloatBuffer::getMonoAsStereo(float *buff, int length) const {
	for (int i = 0; i < length; i++) {
		float s			= (*this)[i];
		buff[i * 2]		= s;
		buff[i * 2 + 1] = s;
	}
}

void FloatBuffer::zeros(size_t N) {
	if (N != 0) {
		resize(N);
	}
//	std::fill(begin(), end(), 0); // slooooow!
#ifdef __APPLE__
	vDSP_vclr(data(), 1, size());
#else
	memset(data(), 0, sizeof(float) * size()); // FAAAASSST!
#endif
}

void FloatBuffer::print() const {
	printf("[");
	for (int i = 0; i < size(); i++) {
		if (i > 0) printf(",");
		printf(" %f", (*this)[i]);
	}
	printf(" ]\n");
}

void FloatBuffer::fadeIn(int length, int numChans, bool smooth) {
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
			"ERROR: FloatBuffer::fadeIn() - trying to fadeIn(%d, %d) on a sample that is only %lu samples long\n",
			length,
			numChans,
			size());
	}
}

void FloatBuffer::fadeOut(int length, int numChans, bool smooth) {
	if (size() >= length * numChans) {
		if (numChans == 1) {
			for (int i = 0; i < length; i++) {
				float fade = i / (float) length;
				if (smooth) fade = smoothstep(fade);
				assignValue(size() - i - 1, (*this)[size() - i - 1] * fade);
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
			"ERROR: FloatBuffer::fadeOut() - trying to fadeOut(%d, %d) on a sample that is only %lu samples long\n",
			length,
			numChans,
			size());
	}
}

//    FloatBuffer& operator= (const FloatBuffer &buff)
//    {
//        // self-assignment guard
//        if (this == &buff)
//            return *this;
//
//        // do the copy
//        assert(size()==buff.size());
//        copy(buff.begin(), buff.end(), this->begin());
//
//        // return the existing object so we can chain this operator
//        return *this;
//    }
FloatBuffer &FloatBuffer::operator+=(const FloatBuffer &right) {
	assert(size() == right.size());
	mix(right);
	return *this;
}

FloatBuffer &FloatBuffer::operator-=(const FloatBuffer &right) {
	assert(size() == right.size());
#ifdef __APPLE__
	vDSP_vsub(this->data(), 1, right.data(), 1, this->data(), 1, this->size());
#else
	for (int i = 0; i < size(); i++) {
		(*this)[i] -= right[i];
	}
#endif
	return *this;
}

FloatBuffer &FloatBuffer::operator*=(const FloatBuffer &right) {
	if (size() != right.size()) {
		//		printf("Wrong size - %ld != %ld\n", size(), right.size());
		assert(size() == right.size());
	}

#ifdef __APPLE__
	vDSP_vmul(this->data(), 1, right.data(), 1, this->data(), 1, this->size());
#else
	for (int i = 0; i < size(); i++) {
		(*this)[i] *= right[i];
	}
#endif

	return *this;
}

void FloatBuffer::stereoGain(float lGain, float rGain) {
//#if 0
#if defined(__APPLE__)

	vDSP_vsmul(data(), 2, &lGain, data(), 2, size() / 2);
	vDSP_vsmul(data() + 1, 2, &rGain, data() + 1, 2, size() / 2);

#elif defined(__ARM_NEON)
	size_t numIterations = size() / 8; // Each iteration processes 4 stereo pairs

	float32x4_t lGainVec = vdupq_n_f32(lGain);
	float32x4_t rGainVec = vdupq_n_f32(rGain);

	for (size_t i = 0; i < numIterations; i++) {
		// Load 4 stereo pairs
		float32x4x2_t stereoPairs = vld2q_f32(data() + i * 8);

		// Multiply left channel values by lGain
		stereoPairs.val[0] = vmulq_f32(stereoPairs.val[0], lGainVec);

		// Multiply right channel values by rGain
		stereoPairs.val[1] = vmulq_f32(stereoPairs.val[1], rGainVec);

		// Store results back
		vst2q_f32(data() + i * 8, stereoPairs);
	}

	// Handle any remaining stereo pairs (if size is not a multiple of 4)
	for (size_t i = numIterations * 8; i < size(); i++) {
		(*this)[i * 2] *= lGain;
		(*this)[i * 2 + 1] *= rGain;
	}
#else
	for (int i = 0; i < size(); i += 2) {
		(*this)[i] *= lGain;
		(*this)[i + 1] *= rGain;
	}
#endif
}

FloatBuffer &FloatBuffer::operator/=(const FloatBuffer &right) {
	assert(size() == right.size());

#ifdef __APPLE__
	vDSP_vdiv(this->data(), 1, right.data(), 1, this->data(), 1, this->size());
#else

	for (int i = 0; i < size(); i++) {
		(*this)[i] /= right[i];
	}
#endif
	return *this;
}

FloatBuffer &FloatBuffer::operator+=(const float &right) {
#ifdef __APPLE__
	vDSP_vsadd(data(), 1, &right, data(), 1, size());

#else
	for (int i = 0; i < size(); i++) {
		(*this)[i] += right;
	}
#endif
	return *this;
}

FloatBuffer &FloatBuffer::operator-=(const float &right) {
#ifdef __APPLE__
	float r = -right;
	vDSP_vsadd(data(), 1, &r, data(), 1, size());

#else
	for (int i = 0; i < size(); i++) {
		(*this)[i] -= right;
	}
#endif
	return *this;
}

FloatBuffer &FloatBuffer::operator*=(const float &right) {
#ifdef __APPLE__
	vDSP_vsmul(data(), 1, &right, data(), 1, size());

#else
	for (int i = 0; i < size(); i++) {
		(*this)[i] *= right;
	}
#endif
	return *this;
}

FloatBuffer &FloatBuffer::operator/=(const float &right) {
#ifdef __APPLE__
	vDSP_vsdiv(data(), 1, &right, data(), 1, size());
#else
	for (int i = 0; i < size(); i++) {
		(*this)[i] /= right;
	}
#endif
	return *this;
}

FloatBuffer operator+(const FloatBuffer &l, const FloatBuffer &r) {
	assert(l.size() == r.size());
	FloatBuffer sum = l;
	sum += r;
	return sum;
}

FloatBuffer operator-(const FloatBuffer &l, const FloatBuffer &r) {
	assert(l.size() == r.size());
	FloatBuffer sum = l;
	sum -= r;
	return sum;
}

void FloatBuffer::clamp(float min, float max) {
#ifdef __APPLE__
	vDSP_vclip(data(), 1, &min, &max, data(), 1, size());
#else
	for (int i = 0; i < size(); i++) {
		if ((*this)[i] < min) (*this)[i] = min;
		else if ((*this)[i] > max) (*this)[i] = max;
	}
#endif
}
void FloatBuffer::convertToMono(int originalNumChannels) {
	int l = (int) size() / originalNumChannels;
	for (int i = 0; i < l; i++) {
		float total = 0.f;
		for (int j = 0; j < originalNumChannels; j++) {
			total += (*this)[i * originalNumChannels + j];
		}
		total /= originalNumChannels;
		(*this)[i] = total;
	}
	resize(l);
}
// linear interpolation
float FloatBuffer::interpolate(double p) const noexcept {
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
	if (a < 0) {
		a = 0;
		b = 1;
	}
	return (*this)[a] * (1.f - m) + (*this)[b] * m;
}

float FloatBuffer::interpolateWrapping(double index) const noexcept {
	int i = index;
	int j = index + 1;
	if (j >= size()) {
		j -= size();
		if (i >= size()) {
			i -= size();
		}
	}
	float frac = index - i;

	return (*this)[i] * (1.f - frac) + (*this)[j] * frac;
}

void FloatBuffer::interpolateStereo(double position, float &outputLeft, float &outputRight) const noexcept {
	auto index1		   = static_cast<int>(std::floor(position));
	auto index2		   = index1 + 1;
	auto coefficient   = static_cast<float>(position - static_cast<double>(index1));
	const auto ourSize = static_cast<int>(size());

	if (index2 * 2 + 1 >= ourSize) {
		if (index1 * 2 + 1 >= ourSize) {
			index1 = index2 = ourSize / 2 - 2;
		} else {
			index2 = index1;
		}
	}

	if (index1 < 0) {
		index1 = 0;
		index2 = 1;
	}

	const size_t left[2]  = {static_cast<size_t>(index1 * 2), static_cast<size_t>(index2 * 2)};
	const size_t right[2] = {static_cast<size_t>(index1 * 2 + 1), static_cast<size_t>(index2 * 2 + 1)};

	if (left[0] >= size() || left[1] >= size() || right[0] >= size() || right[1] >= size()) {
		outputLeft	= 0.f;
		outputRight = 0.f;
		return;
	}

	const auto inverseCoefficient = 1.f - coefficient;

	try {
		outputLeft	= at(left[0]) * inverseCoefficient + at(left[1]) * coefficient;
		outputRight = at(right[0]) * inverseCoefficient + at(right[1]) * coefficient;
	} catch (std::out_of_range const &exc) {
		assert(false);
		outputLeft	= 0.f;
		outputRight = 0.f;
	}
}

// splits this stereo float buffer into 2 mono float buffers passed in param
void FloatBuffer::splitStereo(FloatBuffer &l, FloatBuffer &r) {
	const auto len = size() / 2;

	l.resize(len);
	r.resize(len);

	for (int i = 0; i < len; i++) {
		l[i] = (*this)[i * 2];
		r[i] = (*this)[i * 2 + 1];
	}
}

// combines the 2 params as mono signals into left and right channels of this stereo buffer
void FloatBuffer::combineStereo(const FloatBuffer &l, const FloatBuffer &r) {
	resize(l.size() * 2);
	//mzAssert(l.size()==r.size());

	for (int i = 0; i < l.size(); i++) {
		(*this)[i * 2]	   = l[i];
		(*this)[i * 2 + 1] = r[i];
	}
}

std::optional<size_t> FloatBuffer::findFirstOnset(float threshold) const {
	if (empty()) {
		return std::nullopt;
	}

	size_t sampleCounter = 0;
	for (const auto &sample: *this) {
		if (std::abs(sample) > threshold) {
			return sampleCounter;
		}

		++sampleCounter;
	}

	return std::nullopt;
}