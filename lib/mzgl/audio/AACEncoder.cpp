#include "AACEncoder.h"

#if defined(__ANDROID__)
	#include "androidUtil.h"
#endif

namespace AACEncoder {

    bool encodeAAC(std::string pathToOutput, const FloatBuffer &buff, int numChannels, int sampleRate) {
#if defined(__ANDROID__)
        return androidEncodeAAC(pathToOutput, buff, numChannels, sampleRate);
#endif
        return false;
    }

}