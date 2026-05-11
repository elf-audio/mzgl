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
#include "Result.h"
#include "SampleData.h"

namespace AudioFile {
	// New API: returns the loaded audio (or an error list) by value.
	// SampleData is moved out via RVO so there are no big copies.
	struct LoadedAudio {
		SampleData data;
		int numChannels = 1;
		int sampleRate	= 0;
		Result result;
		explicit operator bool() const noexcept { return result.success(); }
	};

	LoadedAudio load(const std::string &path);
	LoadedAudio loadResampled(const std::string &path, int desiredSampleRate);

	// Legacy out-param overloads. Existing call sites still use these.
	// May throw std::bad_alloc on OOM.
	bool load(std::string path, FloatBuffer &buff, int *outNumChannels, int *outSampleRate = nullptr);
	bool loadResampled(std::string path, FloatBuffer &buff, int newSampleRate, int *outNumChannels);

	// only really for AUv3
	bool load(std::string path, Int16Buffer &buff, int *outNumChannels, int *outSampleRate = nullptr);
	bool loadResampled(std::string path, Int16Buffer &buff, int newSampleRate, int *outNumChannels);
} // namespace AudioFile
