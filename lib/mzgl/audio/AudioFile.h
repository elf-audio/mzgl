//
//  AudioFile.h
//  autosampler
//
//  Created by Marek Bereza on 01/03/2019.
//  Copyright © 2019 Marek Bereza. All rights reserved.
//

#pragma once
#include <string>
#include "FloatBuffer.h"
#include "Int16Buffer.h"

namespace AudioFile {
// these probably throw std::bad_alloc if running out of RAM
bool load(std::string path, FloatBuffer &buff, int *outNumChannels, int *outSampleRate = nullptr);
bool loadResampled(std::string path, FloatBuffer &buff, int newSampleRate, int *outNumChannels);

// only really for AUv3
bool loadResampled(std::string path, Int16Buffer &buff, int newSampleRate, int *outNumChannels);
}

