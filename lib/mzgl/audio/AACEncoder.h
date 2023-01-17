//
//  AACEncoder.h
//  mzgl
//
//  Copyright © 2023 Marek Bereza. All rights reserved.
//

#pragma once


#include <string>
#include <SuperSample.h>

namespace AACEncoder {

    bool encodeAAC(std::string pathToOutput, const FloatBuffer &buff, int numChannels, int sampleRate);

}
