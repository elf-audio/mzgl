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

class AudioFile {
public:
	static bool load(std::string path, FloatBuffer &buff, int *outNumChannels, int *outSampleRate = nullptr);

	static bool loadResampled(std::string path, FloatBuffer &buff, int newSampleRate, int *outNumChannels);
};
