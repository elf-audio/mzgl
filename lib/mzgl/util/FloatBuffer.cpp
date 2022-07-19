
#include "FloatBuffer.h"
#include <assert.h>
//#include "util.h"
#include "maths.h"
#include <fstream>
#include <limits>       // std::numeric_limits
#include <string.h>     // memset()
#ifdef __APPLE__
#include <Accelerate/Accelerate.h>
#endif

using namespace std;


void FloatBuffer::linspace(float a, float b, int N) {
	float h = (b - a) / ((float)N-1.f);
	resize(N);
	for(int i = 0; i < N; i++) {
		(*this)[i] = a + h * i;
	}
}

FloatBuffer::FloatBuffer(size_t sz) {
	zeros(sz);
}

FloatBuffer::FloatBuffer() {
	
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

void FloatBuffer::set(const float *buff, size_t length, int start, int stride) {
	
	if(start==0 && stride==1) {
		assign(buff, buff + length);
	} else {
		resize(length);
		int j = start;
		for(int i = 0; i < length; i++) {
			(*this)[i] = buff[j];
			j += stride;
		}
	}
}
void FloatBuffer::setMonoFromStereo(const float *buff, size_t numFrames) {
	resize(numFrames);
	for(int i = 0; i < numFrames; i++) {
		(*this)[i] = (buff[i*2] + buff[i*2+1])*0.5f;
	}
}
void FloatBuffer::setStereoFromMono(float *data, int length) {
	resize(length*2);
	for(int i = 0; i < length; i++) {
		(*this)[i*2] = data[i];
		(*this)[i*2+1] = data[i];
	}
}

int16_t FloatBuffer::floatToInt16(float in) const {
	return in * 32767.f;
}
vector<int16_t> FloatBuffer::getInt16() const {
	vector<int16_t> buff;
	buff.resize(size());
	for(int i = 0; i < size(); i++) {
		buff[i] = floatToInt16((*this)[i]);
	}
	return buff;
}

FloatBuffer FloatBuffer::stereoToMono() {
	FloatBuffer ret;
	ret.resize(size() / 2);
	for(int i = 0; i < ret.size(); i++) {
		ret[i] = ((*this)[i*2] + (*this)[i*2+1]) * 0.5;
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
//	ifstream myfile (path.c_str());
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
	for(int i = 0; i < size(); i++) {
		total += (*this)[i];
	}
	return total;
}
void FloatBuffer::square() {
	for(int i = 0; i < size(); i++) {
		(*this)[i] *= (*this)[i];
	}
}
float FloatBuffer::mean() const {
	return sum() / (float) size();
}

float FloatBuffer::absMax() const {
	return abs(at(findAbsMaxPos()));
}

float FloatBuffer::maxValue() const {
#ifdef __APPLE__
	float maxVal = -INFINITY;
	vDSP_Length pos;
	vDSP_maxvi(data(), 1, &maxVal,  &pos, size());
	return maxVal;
#else
	  
	return at(findMaxPos());
#endif
}

float FloatBuffer::minValue() const {
	
#ifdef __APPLE__
	float minVal = INFINITY;
	vDSP_Length pos;
	vDSP_maxvi(data(), 1, &minVal,  &pos, size());
	return minVal;
#else
	return at(findMinPos());
#endif
}


int FloatBuffer::findAbsMaxPos() const {
	float max = 0;
	int pos = 0;
	for(int i = 0; i < size(); i++) {
		if(abs(at(i))>max) {
			max = abs(at(i));
			pos = i;
		}
	}
	return pos;
}

int FloatBuffer::findMaxPos() const {
	float maxVal = std::numeric_limits<float>::min();
#ifdef __APPLE__
	vDSP_Length pos;
	vDSP_maxvi(data(), 1, &maxVal,  &pos, size());
#else
	int pos = 0;
	for(int i = 0; i < size(); i++) {
		if(at(i)>maxVal) {
			maxVal = at(i);
			pos = i;
		}
	}
#endif
	return pos;
}

int FloatBuffer::findMinPos() const {
	
	float minVal = std::numeric_limits<float>::max();
#ifdef __APPLE__
	vDSP_Length pos;
	vDSP_maxvi(data(), 1, &minVal,  &pos, size());
	return minVal;
#else
	int pos = 0;
	for(int i = 0; i < size(); i++) {
		if(at(i)<minVal) {
			minVal = at(i);
			pos = i;
		}
	}
#endif
	return pos;
}

void FloatBuffer::getMinMax(float &min, float &max) const {
	if(size()==0) return;
	min = std::numeric_limits<float>::max();
	max = std::numeric_limits<float>::min();
	
	for(int i = 0; i < size(); i++) {
		if(at(i)<min) {
			min = at(i);
		}
		if(at(i)>max) {
			max = at(i);
		}
	}
}

void FloatBuffer::normalize(float min, float max) {
	float inMin, inMax;
	getMinMax(inMin, inMax);
	
	float norm = 1.f / (inMax - inMin);
	float scale = max - min;
	
	for(int i = 0; i < size(); i++) {
		(*this)[i] = min + scale * (at(i) - inMin) * norm;
	}
}


void FloatBuffer::normalizeAudio() {
	
	float inMin, inMax;
	getMinMax(inMin, inMax);
	
	float loudest = std::max(abs(inMin), abs(inMax));
	if(loudest<0.000001) {
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
	
	// make sure both samples are
	// as long as the longest one
	if(size()<other.size()) {
		resize(other.size(), 0.f);
	}
	
	// TODO: Accelerate
	for(int i = 0; i < other.size(); i++) {
		(*this)[i] += other[i];
	}
}


void FloatBuffer::random(float min, float max) {
	for(int i = 0; i < size(); i++) {
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
	for(int i =0 ; i < length; i++) {
		float s = (*this)[i];
		buff[i*2] = s;
		buff[i*2+1] = s;
	}
}

void FloatBuffer::zeros(size_t N) {
	if(N!=0) {
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
	for(int i = 0; i < size(); i++) {
		if(i>0) printf(",");
		printf(" %f", (*this)[i]);
	}
	printf(" ]\n");
}

void FloatBuffer::fadeIn(int length, int numChans) {
	if(size()*numChans>length) {
		
		if(numChans==1) {
			for(int i = 0; i < length; i++) {
				assignValue(i, (*this)[i] * i/(float)length);
			}
		} else if(numChans==2) {
			for(int i = 0; i < length; i++) {
				const float fade = i/(float)length;
				assignValue(i*2, (*this)[i*2]*fade);
				assignValue(i*2+1, (*this)[i*2+1]*fade);
			}
		} else {
			for(int i = 0; i < length; i++) {
				const float fade = i/(float)length;
				for(int ch = 0; ch < numChans; ch++) {
					assignValue(i*numChans + ch, (*this)[i*numChans + ch]*fade);
				}
			}
		}
	} else {
		printf("ERROR: FloatBuffer::fadeIn() - trying to fadeIn(%d, %d) on a sample that is only %lu samples long\n", length, numChans, size());
	}
}


void FloatBuffer::fadeOut(int length, int numChans) {
	if(size()>=length*numChans) {
		if(numChans==1) {
			for(int i = 0; i < length; i++) {
				assignValue(size() - i - 1, (*this)[size() - i - 1] * i/(float)length);
			}
		} else if(numChans==2) {
			for(int i = 0; i < length; i++) {
				float fade = i/(float)length;
				int frameIndex = ((int)size() / numChans) - i - 1;
				assignValue(frameIndex*2, (*this)[frameIndex*2] * fade);
				assignValue(frameIndex*2+1, (*this)[frameIndex*2+1] * fade);
			}
		} else {
			for(int i = 0; i < length; i++) {
				float fade = i/(float)length;
				int frameIndex = ((int)size() / numChans) - i - 1;
				for(int ch = 0; ch < numChans; ch++) {
					assignValue(frameIndex*numChans+ch, (*this)[frameIndex*numChans+ch] * fade);
				}
			}
		}
	} else {
		printf("ERROR: FloatBuffer::fadeOut() - trying to fadeOut(%d, %d) on a sample that is only %lu samples long\n", length, numChans, size());
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
FloatBuffer& FloatBuffer::operator+=(const FloatBuffer& right) {
	assert(size()==right.size());
    
#ifdef __APPLE__
    vDSP_vadd(this->data(), 1,
              right.data(), 1,
              this->data(), 1,
              this->size());
#else
	for(int i = 0; i < size(); i++) {
		(*this)[i] += right[i];
	}
#endif
	return *this;
}

FloatBuffer& FloatBuffer::operator-=(const FloatBuffer& right) {
	assert(size()==right.size());
#ifdef __APPLE__
    vDSP_vsub(this->data(), 1,
              right.data(), 1,
              this->data(), 1,
              this->size());
#else
	for(int i = 0; i < size(); i++) {
		(*this)[i] -= right[i];
	}
#endif
	return *this;
}

FloatBuffer& FloatBuffer::operator*=(const FloatBuffer& right) {
	if(size()!=right.size()) {
		printf("Wrong size - %ld != %ld\n", size(),  right.size());
		assert(size()==right.size());
	}
	
#ifdef __APPLE__
	vDSP_vmul(this->data(), 1,
			  right.data(), 1,
			  this->data(), 1,
			  this->size());
#else
	for(int i = 0; i < size(); i++) {
		(*this)[i] *= right[i];
	}
#endif
	
	
	return *this;
}

FloatBuffer& FloatBuffer::operator/=(const FloatBuffer& right) {
	assert(size()==right.size());
    
#ifdef __APPLE__
    vDSP_vdiv(this->data(), 1,
              right.data(), 1,
              this->data(), 1,
              this->size());
#else

	for(int i = 0; i < size(); i++) {
		(*this)[i] /= right[i];
	}
#endif
	return *this;
}


FloatBuffer& FloatBuffer::operator+=(const float& right) {
#ifdef __APPLE__
    vDSP_vsadd(data(), 1, &right, data(), 1, size());

#else
	for(int i = 0; i < size(); i++) {
		(*this)[i] += right;
	}
#endif
	return *this;
}

FloatBuffer& FloatBuffer::operator-=(const float& right) {
#ifdef __APPLE__
    float r = -right;
    vDSP_vsadd(data(), 1, &r, data(), 1, size());

#else
	for(int i = 0; i < size(); i++) {
		(*this)[i] -= right;
	}
#endif
	return *this;
}

FloatBuffer& FloatBuffer::operator*=(const float& right) {
    
#ifdef __APPLE__
    vDSP_vsmul(data(), 1, &right, data(), 1, size());

#else
	for(int i = 0; i < size(); i++) {
		(*this)[i] *= right;
	}
#endif
	return *this;
}

FloatBuffer& FloatBuffer::operator/=(const float& right) {
#ifdef __APPLE__
    vDSP_vsdiv(data(), 1, &right, data(), 1, size());

#else
	for(int i = 0; i < size(); i++) {
		(*this)[i] /= right;
	}
#endif
	return *this;
}

FloatBuffer operator+(const FloatBuffer &l, const FloatBuffer &r) {
	assert(l.size()==r.size());
	FloatBuffer sum = l;
	sum += r;
	return sum;
	
}

FloatBuffer operator-(const FloatBuffer &l, const FloatBuffer &r) {
	assert(l.size()==r.size());
	FloatBuffer sum = l;
	sum -= r;
	return sum;
	
}


void FloatBuffer::clamp(float min, float max) {
#ifdef __APPLE__
    vDSP_vclip(data(), 1, &min, &max, data(), 1, size());
#else
    for(int i =0; i < size(); i++) {
        if((*this)[i]<min) (*this)[i] = min;
        else if((*this)[i]>max) (*this)[i] = max;
    }
#endif
}
void FloatBuffer::convertToMono(int originalNumChannels) {
	int l = (int)size() / originalNumChannels;
	for(int i = 0; i < l; i++) {
		float total = 0.f;
		for(int j = 0; j < originalNumChannels; j++) {
			total += (*this)[i*originalNumChannels + j];
		}
		total /= originalNumChannels;
		(*this)[i] = total;
	}
	resize(l);
}
// linear interpolation
float FloatBuffer::interpolate(double p) const noexcept {
	int a = p;
	int b = a + 1;
	float m = p - a;
	if(b>=size()) {
		if(a>=size()) {
			a = b = (int)size()-1;
		} else {
			b = a;
		}
	}
	return (*this)[a]*(1.f - m) + (*this)[b] * m;
}

void FloatBuffer::interpolateStereo(double p, float &L, float &R) const noexcept {
	int a = p;
	int b = a + 1;
	float m = p - a;
	if(b*2+1>=size()) {
		if(a*2+1>=size()) {
			a = b = (int)size()/2 - 2;
		} else {
			b = a;
		}
	}
#ifdef DEBUG
	if(isnan((*this)[a*2])) {
		printf("Nan a*2 - pos: %d, size %lu\n", a*2, size());
	} else if(isnan((*this)[a*2+1])) {
		printf("Nan a*2+1 - pos: %d, size %lu\n", a*2+1, size());
	} else if(isnan((*this)[b*2])) {
		printf("Nan b*2 - pos: %d, size %lu\n", b*2, size());
	} else if(isnan((*this)[b*2+1])) {
		printf("Nan b*2+1 - pos: %d, size %lu\n", b*2+1, size());
	}
#endif
	L = (*this)[a*2]*(1.f - m) + (*this)[b*2] * m;
	R = (*this)[a*2+1]*(1.f - m) + (*this)[b*2+1] * m;
}
