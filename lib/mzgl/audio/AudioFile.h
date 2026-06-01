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
	// Bitmask flags describing which exception path the noexcept loader
	// caught. Multiple flags can be set on the same LoadedAudio if both
	// the load itself and the subsequent issue-allocation failed (e.g.
	// FilesystemError | OutOfMemory). LoadedAudio::operator bool returns
	// false whenever any bit is set, regardless of result.success().
	namespace LoadFailureFlag {
		constexpr int None			  = 0;
		constexpr int OutOfMemory	  = 1 << 0; // std::bad_alloc
		constexpr int FilesystemError = 1 << 1; // std::filesystem::filesystem_error
		constexpr int SystemError	  = 1 << 2; // std::system_error
		constexpr int Exception		  = 1 << 3; // any other std::exception
		constexpr int Unknown		  = 1 << 4; // non-std exception
	} // namespace LoadFailureFlag

	// New API: returns the loaded audio (or an error list) by value.
	// SampleData is moved out via RVO so there are no big copies.
	struct LoadedAudio {
		SampleData data;
		int numChannels = 1;
		int sampleRate	= 0;
		Result result;
		int failureFlags = LoadFailureFlag::None;
		explicit operator bool() const noexcept {
			return failureFlags == LoadFailureFlag::None && result.success();
		}
	};

	LoadedAudio load(const std::string &path) noexcept;
	LoadedAudio loadResampled(const std::string &path, int desiredSampleRate) noexcept;

	// Same contract as load / loadResampled but always routes through the
	// dr_libs decoder path (dr_wav / dr_flac / dr_mp3 + AIFF reader). On
	// Linux, Windows, and Android this is what load() does for those
	// formats anyway; on Apple it's a distinct alternative useful for
	// testing parity. Formats not handled by dr_libs (mp4/aac/alac) fail.
	LoadedAudio loadViaDrLib(const std::string &path) noexcept;
	LoadedAudio loadResampledViaDrLib(const std::string &path,
									  int desiredSampleRate) noexcept;

	// Legacy out-param overloads. Existing call sites still use these.
	// May throw std::bad_alloc on OOM.
	bool load(std::string path, FloatBuffer &buff, int *outNumChannels, int *outSampleRate = nullptr);
	bool loadResampled(std::string path, FloatBuffer &buff, int newSampleRate, int *outNumChannels);

	// only really for AUv3
	bool load(std::string path, Int16Buffer &buff, int *outNumChannels, int *outSampleRate = nullptr);
	bool loadResampled(std::string path, Int16Buffer &buff, int newSampleRate, int *outNumChannels);
} // namespace AudioFile
