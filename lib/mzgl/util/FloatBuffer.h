/**     ___           ___           ___                         ___           ___
 *     /__/\         /  /\         /  /\         _____         /  /\         /__/|
 *    |  |::\       /  /::\       /  /::|       /  /::\       /  /::\       |  |:|
 *    |  |:|:\     /  /:/\:\     /  /:/:|      /  /:/\:\     /  /:/\:\      |  |:|
 *  __|__|:|\:\   /  /:/~/::\   /  /:/|:|__   /  /:/~/::\   /  /:/  \:\   __|__|:|
 * /__/::::| \:\ /__/:/ /:/\:\ /__/:/ |:| /\ /__/:/ /:/\:| /__/:/ \__\:\ /__/::::\____
 * \  \:\~~\__\/ \  \:\/:/__\/ \__\/  |:|/:/ \  \:\/:/~/:/ \  \:\ /  /:/    ~\~~\::::/
 *  \  \:\        \  \::/          |  |:/:/   \  \::/ /:/   \  \:\  /:/      |~~|:|~~
 *   \  \:\        \  \:\          |  |::/     \  \:\/:/     \  \:\/:/       |  |:|
 *    \  \:\        \  \:\         |  |:/       \  \::/       \  \::/        |  |:|
 *     \__\/         \__\/         |__|/         \__\/         \__\/         |__|/
 *
 *  Description:
 *
 *	sustainPedal2
 *
 *  FloatBuffer.h, created by Marek Bereza on 01/08/2017.
 *
 */
#pragma once
#include <vector>
#include <string>
#include <memory>
#include <optional>
//#include "xsimd/xsimd.hpp"

//class FloatBuffer : public std::vector<float, xsimd::aligned_allocator<float>> {
// class FloatBuffer : public std::vector<float, AlignedAllocator<float>> {
class FloatBuffer : public std::vector<float> {
public:
	static constexpr auto defaultThresholdForOnsets = 0.05;

	FloatBuffer(size_t sz);

	FloatBuffer() = default;

	// Copy constructor
	FloatBuffer(const FloatBuffer &other)
		: std::vector<float>(other) {}

	// Copy assignment operator
	FloatBuffer &operator=(const FloatBuffer &other) {
		std::vector<float>::operator=(other);
		return *this;
	}

	FloatBuffer &operator=(FloatBuffer &&other) noexcept {
		std::vector<float>::operator=(std::move(other));
		return *this;
	}

	FloatBuffer(FloatBuffer &&other) noexcept
		: std::vector<float>(std::move(other)) {}

	virtual ~FloatBuffer();

	FloatBuffer(const float *arr, size_t size);

	// xsimd disabled for MSVC build, it does not give performance boost for Android anyway - to be investigated
	//FloatBuffer(size_t size, float fillValue) : std::vector<float, xsimd::aligned_allocator<float>>(size, fillValue) {}

	FloatBuffer(size_t size, float fillValue)
		: std::vector<float>(size, fillValue) {}

	/**
     * You can copy an array of floats into the buffer
     * by doing set(buff, length);
     * or if you want to get just the left channel, you can
     * set the start to 0 and the stride to 2
     * or right channel with start=1 and stride = 2.
     *
     * parameters:
     *  length is always how long you want your buffer to be
     *  start is the offset in the input float array
     *  stride is how many samples to iterate by on each loop
     */
	void set(const float *buff, size_t length, int start = 0, int stride = 1);
	void setMonoFromStereo(const float *buff, size_t numFrames);
	void setMonoFromStereo(const FloatBuffer &b) { setMonoFromStereo(b.data(), b.size() / 2); }
	void setStereoFromMono(const float *data, int length);
	void setStereoFromMono(const FloatBuffer &b) { setStereoFromMono(b.data(), static_cast<int>(b.size())); }
	void setFromLeftChannel(float *buff, int length);
	void setFromRightChannel(float *buff, int length);

	// splits this stereo float buffer into 2 mono float buffers passed in param
	void splitStereo(FloatBuffer &l, FloatBuffer &r);

	// combines the 2 params as mono signals into left and right channels of this stereo buffer
	void combineStereo(const FloatBuffer &l, const FloatBuffer &r);

	void assignValue(size_t index, float v) { (*this)[index] = v; }

	// if smooth = false, fadeout is linear, if it's true, then its smoothed
	// using the smoothstep algorithm.
	void fadeIn(int length, int numChans, bool smooth = false);
	void fadeOut(int length, int numChans, bool smooth = false);

	/**
	 * This copies a section out of this float buffer and
	 * returns it as another floatbuffer (outBuff in parameter)
	 * end is exclusive
	 */
	void splice(int start, int end, FloatBuffer &outBuff) const;
	FloatBuffer splice(int start, int end) const;

	int16_t floatToInt16(float in) const;
	vector<int16_t> getInt16() const;

	FloatBuffer stereoToMono();

	void convertToMono(int originalNumChannels = 2);

	//	void save(std::string path);
	//	void load(std::string path);

	float sum() const;
	float mean() const;

	// square each value
	void square();

	float absMax() const;
	float maxValue() const;
	float minValue() const;

	int findAbsMaxPos() const;
	int findMaxPos() const;
	int findMinPos() const;

	void getMinMax(float &min, float &max) const;

	void zeros(size_t N = 0);
	void random(float min = -1, float max = 1);
	void linspace(float a, float b, int N);

	void mix(const FloatBuffer &other);
	void mixNaive(const FloatBuffer &other); // no optimization for neon testing

	void normalize(float min = -1, float max = 1);

	// this is like normalize but it doesn't cause a DC offset
	void normalizeAudio();

	void clamp(float min = -1, float max = 1);

	void append(const float *buff, int length);
	void append(const FloatBuffer &b);

	// multiplies the buffer by left and right gain if it's
	// a stereo interleaved buffer.
	void stereoGain(float lGain, float rGain);

	// sets the input samples to the contents of this FloatBuffer

	void get(float *buff) const;
	void get(float *buff, int len) const;
	void getMonoAsStereo(float *buff, int length) const;

	// linear interpolation
	float interpolate(double index) const noexcept;

	void interpolateStereo(double index, float &L, float &R) const noexcept;

	// for delay lines
	float interpolateWrapping(double index) const noexcept;

	void print() const;

	FloatBuffer &operator+=(const FloatBuffer &right);
	FloatBuffer &operator-=(const FloatBuffer &right);
	FloatBuffer &operator*=(const FloatBuffer &right);
	FloatBuffer &operator/=(const FloatBuffer &right);
	FloatBuffer &operator+=(const float &right);
	FloatBuffer &operator-=(const float &right);
	FloatBuffer &operator*=(const float &right);
	FloatBuffer &operator/=(const float &right);

	std::optional<size_t> findFirstOnset(float threshold = defaultThresholdForOnsets) const;
};

FloatBuffer operator+(const FloatBuffer &l, const FloatBuffer &r);
FloatBuffer operator-(const FloatBuffer &l, const FloatBuffer &r);

using FloatBufferRef = std::shared_ptr<FloatBuffer>;
