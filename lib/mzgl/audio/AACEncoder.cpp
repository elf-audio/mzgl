#include "AACEncoder.h"

#if defined(__ANDROID__)
	#include "androidUtil.h"
#endif

namespace AACEncoder {

    void encodeAAC(std::string pathToOutput, const FloatBuffer &buff, int numChannels, int sampleRate) {
#if defined(__ANDROID__)
        androidEncodeAAC(pathToOutput, buff, numChannels, sampleRate);
#endif
    }
}