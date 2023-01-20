
#pragma once


#include <string>
#include "FloatBuffer.h"

namespace AACEncoder {
    bool encodeAAC(std::string pathToOutput, const FloatBuffer &buff, int numChannels, int sampleRate);
}
