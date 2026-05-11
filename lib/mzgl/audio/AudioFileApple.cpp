//
//  AudioFile.cpp
//  MZGL
//
//  Created by Marek Bereza on 01/03/2019.
//  Copyright © 2019 Marek Bereza. All rights reserved.
//

#include "AudioFile.h"
#ifdef __APPLE__
#	include <CoreFoundation/CoreFoundation.h>
#	include <AudioToolbox/AudioToolbox.h>
#	include "mzAssert.h"
#	include <optional>
#	include <string>
#	include "log.h"

std::string stringFromOSStatus(OSStatus status) {
	switch (status) {
		case noErr: return "No error";
		case kAudioFileInvalidFileError: return "The file is malformed or not recognized as a valid audio file";
		case kAudioFileUnspecifiedError: return "An unspecified error occurred";
		case kAudioFileUnsupportedFileTypeError: return "The file type is not supported or recognized";
		case kAudioFileUnsupportedDataFormatError:
			return "The data format in the file is not supported or recognized";
		case kAudioFilePermissionsError: return "The file permissions do not allow the operation";
		case kAudioFileNotOptimizedError: return "The file is not optimized for the operation";
		case kAudioFileInvalidPacketOffsetError: return "The packet offset is not valid or supported";
		default: return "Unknown error: " + std::to_string(int(status));
	}
}

class Url {
public:
	explicit Url(const std::string &path)
		: url {CFURLCreateWithFileSystemPath(
			  kCFAllocatorDefault,
			  CFStringCreateWithCString(kCFAllocatorDefault, path.c_str(), kCFStringEncodingUTF8),
			  kCFURLPOSIXPathStyle,
			  false)} {}
	~Url() { CFRelease(url); }

	CFURLRef url;
};

class AudioFileRef {
public:
	AudioFileRef() = default;
	~AudioFileRef() {
		if (file != nullptr) {
			ExtAudioFileDispose(file);
		}
	}

	[[nodiscard]] bool checkStatus(OSStatus result, const std::string &action) const {
		if (result == noErr) {
			return true;
		}
		Log::e() << "Got an error whilst " + action + " -> " + stringFromOSStatus(result);
		return false;
	}

	[[nodiscard]] bool open(const Url &url) {
		try {
			return checkStatus(ExtAudioFileOpenURL(url.url, &file), "opening");
		} catch (...) {
			Log::e() << "Caught exception whilst opening ExtAudioFile";
		}
		return false;
	}

	[[nodiscard]] bool getInputFormat(AudioStreamBasicDescription &inputFormat) {
		UInt32 propertySize = sizeof(inputFormat);
		return checkStatus(
			ExtAudioFileGetProperty(file, kExtAudioFileProperty_FileDataFormat, &propertySize, &inputFormat),
			"getting input format");
	}

	void getReadFormat(AudioStreamBasicDescription &audioFormat, Float64 sampleRate, UInt32 numberOfChannels) {
		audioFormat.mSampleRate		  = sampleRate;
		audioFormat.mFormatID		  = kAudioFormatLinearPCM;
		audioFormat.mFormatFlags	  = kLinearPCMFormatFlagIsFloat;
		audioFormat.mBitsPerChannel	  = sizeof(Float32) * 8;
		audioFormat.mChannelsPerFrame = numberOfChannels;
		audioFormat.mBytesPerFrame	  = audioFormat.mChannelsPerFrame * sizeof(Float32);
		audioFormat.mFramesPerPacket  = 1;
		audioFormat.mBytesPerPacket	  = audioFormat.mFramesPerPacket * audioFormat.mBytesPerFrame;
	}

	[[nodiscard]] bool getFrameCount(SInt64 &frameCount) {
		UInt32 propertySize = sizeof(frameCount);
		return checkStatus(
			ExtAudioFileGetProperty(file, kExtAudioFileProperty_FileLengthFrames, &propertySize, &frameCount),
			"getting frame count");
	}

	[[nodiscard]] bool applyReadFormat(AudioStreamBasicDescription &audioFormat) {
		return checkStatus(
			ExtAudioFileSetProperty(
				file, kExtAudioFileProperty_ClientDataFormat, sizeof(AudioStreamBasicDescription), &audioFormat),
			"applying read format");
	}

	template <class Buffer>
	[[nodiscard]] bool read(AudioStreamBasicDescription &audioFormat, Buffer &buff) {
		static constexpr UInt32 outputSizeInKb = 32;
		static constexpr UInt32 bytePerKb	   = 1024;

		auto outputBufferSize = outputSizeInKb * bytePerKb;
		auto packetsPerBuffer = outputBufferSize / audioFormat.mBytesPerPacket;

		std::vector<uint8_t> outputBuffer(outputBufferSize, 0);
		AudioBufferList convertedData;
		convertedData.mNumberBuffers			  = 1;
		convertedData.mBuffers[0].mNumberChannels = audioFormat.mChannelsPerFrame;
		convertedData.mBuffers[0].mDataByteSize	  = outputBufferSize;
		convertedData.mBuffers[0].mData			  = outputBuffer.data();

		// this function doesn't give a 100% accurate frame count - with wav files sometimes its 2 samples too short
		// but its close enough for us at the moment.
		bool done = false;
		while (!done) {
			auto frameCount = packetsPerBuffer;

			convertedData.mBuffers[0].mDataByteSize = outputBufferSize;
			if (!checkStatus(ExtAudioFileRead(file, &frameCount, &convertedData), "reading packet")) {
				return false;
			}

			buff.append((float *) convertedData.mBuffers[0].mData, frameCount * audioFormat.mChannelsPerFrame);
			done = frameCount == 0;
		}
		return true;
	}

	ExtAudioFileRef file {nullptr};
};

#endif

#include <fstream>

#include "filesystem.h"

#include "util.h"
#include "log.h"

/**
 * This checks the magic numbers of a file if it has .mp3 extension.
 * If it turns out to be an m4a, it will add .m4a to the end of the path
 */
static std::string checkItsNotAnMp4PretendingToBeAnMp3(std::string path) {
	fs::path p(path);
	if (p.extension() != ".mp3") {
		return path;
	}

	constexpr int bytesToRead = 11;
	if (fs::file_size(p) > bytesToRead) {
		char data[bytesToRead];
		std::ifstream fstr;
		fstr.open(path);
		fstr.read(data, bytesToRead);
		fstr.close();

		if (data[0] == 0 && data[1] == 0 && data[2] == 0 && data[4] == 'f' && data[5] == 't' && data[6] == 'y'
			&& data[7] == 'p') {
			auto outPath = tempDir() + "/" + p.filename().string() + ".m4a";
			try {
				fs::copy(path, outPath);
				return outPath;
			} catch (fs::filesystem_error &err) {
				Log::e() << "Couldn't copy mp3 to m4a";
				return path;
			}
		}
	}

	return path;
}

template <class Buffer>
bool loadAudioFile(
	std::string path, Buffer &buff, std::optional<int> newSampleRate, int *outNumChannels, int *outSampleRate) {
#ifdef __APPLE__
	path = checkItsNotAnMp4PretendingToBeAnMp3(path);

	if (!fs::exists(fs::path {path})) {
		return false;
	}

	auto status = fs::status(fs::path {path});
	if (!((status.permissions() & fs::perms::owner_read) != fs::perms::none
		  || (status.permissions() & fs::perms::group_read) != fs::perms::none
		  || (status.permissions() & fs::perms::others_read) != fs::perms::none)) {
		return false;
	}

	Url url {path};
	AudioFileRef file;
	if (!file.open(url)) {
		Log::d() << "Couldn't open file on " << path;
		return false;
	}

	AudioStreamBasicDescription inputFormat;
	if (!file.getInputFormat(inputFormat)) {
		Log::d() << "Couldn't get input format for " << path;
		return false;
	}

	SInt64 frameCount = 0;
	if (file.getFrameCount(frameCount) && frameCount == 0) {
		if (outNumChannels != nullptr) {
			*outNumChannels = (unsigned) inputFormat.mChannelsPerFrame;
		}
		if (outSampleRate != nullptr) {
			*outSampleRate = inputFormat.mSampleRate;
		}
		return true;
	}

	AudioStreamBasicDescription audioFormat;
	file.getReadFormat(audioFormat,
					   newSampleRate.has_value() ? *newSampleRate : inputFormat.mSampleRate,
					   (unsigned) inputFormat.mChannelsPerFrame);

	if (!file.applyReadFormat(audioFormat)) {
		Log::d() << "Could not prepare the read format for " << path;
		return false;
	}

	if (!file.read(audioFormat, buff)) {
		Log::d() << "Failed to read audio file on " << path;
		return false;
	}

#endif
	if (outNumChannels != nullptr) {
		*outNumChannels = (unsigned) inputFormat.mChannelsPerFrame;
	}

	if (outSampleRate != nullptr) {
		*outSampleRate = inputFormat.mSampleRate;
	}

	return true;
}

bool AudioFile::loadResampled(std::string path, Int16Buffer &buff, int newSampleRate, int *outNumChannels) {
	return loadAudioFile(path, buff, newSampleRate, outNumChannels, nullptr);
}

bool AudioFile::loadResampled(std::string path, FloatBuffer &buff, int newSampleRate, int *outNumChannels) {
	return loadAudioFile<FloatBuffer>(path, buff, newSampleRate, outNumChannels, nullptr);
}

bool AudioFile::load(std::string path, FloatBuffer &buff, int *outNumChannels, int *outSampleRate) {
	return loadAudioFile(path, buff, std::nullopt, outNumChannels, outSampleRate);
}
bool AudioFile::load(std::string path, Int16Buffer &buff, int *outNumChannels, int *outSampleRate) {
	return loadAudioFile(path, buff, std::nullopt, outNumChannels, outSampleRate);
}

namespace {
	// Pick a SampleFormat that matches the file's native PCM bit depth.
	// Returns std::nullopt if the file is non-PCM (mp3/m4a/aac/etc) or
	// uses an unsupported PCM layout — caller falls back to F32 then.
	std::optional<SampleFormat> nativeFormatFor(const AudioStreamBasicDescription &fmt) {
		if (fmt.mFormatID != kAudioFormatLinearPCM) return std::nullopt;
		const bool isFloat	= (fmt.mFormatFlags & kLinearPCMFormatFlagIsFloat) != 0;
		const bool isInt	= (fmt.mFormatFlags & kLinearPCMFormatFlagIsSignedInteger) != 0;
		const UInt32 bits	= fmt.mBitsPerChannel;
		if (isFloat && bits == 32) return SampleFormat::F32;
		if (isInt && bits == 8) return SampleFormat::I8;
		if (isInt && bits == 16) return SampleFormat::I16;
		if (isInt && bits == 24) return SampleFormat::I24;
		// 32-bit int is uncommon; promote to F32 to keep dynamic range.
		return std::nullopt;
	}

	void fillNativePCMReadFormat(AudioStreamBasicDescription &out,
								 SampleFormat fmt,
								 Float64 sampleRate,
								 UInt32 numberOfChannels) {
		out				 = {};
		out.mSampleRate	 = sampleRate;
		out.mFormatID	 = kAudioFormatLinearPCM;
		out.mChannelsPerFrame = numberOfChannels;
		switch (fmt) {
			case SampleFormat::I8:
				out.mFormatFlags  = kAudioFormatFlagIsSignedInteger | kAudioFormatFlagIsPacked;
				out.mBitsPerChannel = 8;
				break;
			case SampleFormat::I16:
				out.mFormatFlags  = kAudioFormatFlagIsSignedInteger | kAudioFormatFlagIsPacked;
				out.mBitsPerChannel = 16;
				break;
			case SampleFormat::I24:
				out.mFormatFlags  = kAudioFormatFlagIsSignedInteger | kAudioFormatFlagIsPacked;
				out.mBitsPerChannel = 24;
				break;
			case SampleFormat::F32:
				out.mFormatFlags  = kLinearPCMFormatFlagIsFloat | kAudioFormatFlagIsPacked;
				out.mBitsPerChannel = 32;
				break;
		}
		out.mBytesPerFrame	 = numberOfChannels * (out.mBitsPerChannel / 8);
		out.mFramesPerPacket = 1;
		out.mBytesPerPacket	 = out.mBytesPerFrame;
	}

	AudioFile::LoadedAudio loadInto(const std::string &path, std::optional<int> resampleTo) {
		AudioFile::LoadedAudio out;

		const std::string resolvedPath = checkItsNotAnMp4PretendingToBeAnMp3(path);
		if (!fs::exists(fs::path {resolvedPath})) {
			out.result.addIssue("Path does not exist: " + resolvedPath);
			return out;
		}

		Url url {resolvedPath};
		AudioFileRef file;
		if (!file.open(url)) {
			out.result.addIssue("Couldn't open " + resolvedPath);
			return out;
		}

		AudioStreamBasicDescription inputFormat;
		if (!file.getInputFormat(inputFormat)) {
			out.result.addIssue("Couldn't read input format for " + resolvedPath);
			return out;
		}

		out.numChannels = static_cast<int>(inputFormat.mChannelsPerFrame);
		out.sampleRate	= resampleTo.value_or(static_cast<int>(inputFormat.mSampleRate));

		SInt64 frameCount = 0;
		if (file.getFrameCount(frameCount) && frameCount == 0) {
			return out;
		}

		// Resampled loads always go through float. But if the caller
		// asked to resample to the same rate the file is already at (e.g.
		// dragging a 48 kHz wav into a 48 kHz engine), treat it as a
		// no-op and keep the native bit depth.
		const bool noResampleNeeded =
			!resampleTo.has_value()
			|| static_cast<int>(inputFormat.mSampleRate) == *resampleTo;
		const std::optional<SampleFormat> targetFmt =
			noResampleNeeded ? nativeFormatFor(inputFormat) : std::nullopt;

		if (!targetFmt.has_value()) {
			AudioStreamBasicDescription audioFormat;
			file.getReadFormat(audioFormat,
							   resampleTo.value_or(inputFormat.mSampleRate),
							   inputFormat.mChannelsPerFrame);

			if (!file.applyReadFormat(audioFormat)) {
				out.result.addIssue("Could not prepare the read format for " + resolvedPath);
				return out;
			}
			FloatBuffer tmp;
			if (!file.read(audioFormat, tmp)) {
				out.result.addIssue("Failed to read audio file " + resolvedPath);
				return out;
			}
			out.data = SampleData(std::move(tmp));
			return out;
		}

		AudioStreamBasicDescription audioFormat;
		fillNativePCMReadFormat(
			audioFormat, *targetFmt, inputFormat.mSampleRate, inputFormat.mChannelsPerFrame);

		if (!file.applyReadFormat(audioFormat)) {
			// Native format not honored by the converter — fall back to f32.
			AudioStreamBasicDescription fallback;
			file.getReadFormat(fallback, inputFormat.mSampleRate, inputFormat.mChannelsPerFrame);
			if (!file.applyReadFormat(fallback)) {
				out.result.addIssue("Could not prepare any read format for " + resolvedPath);
				return out;
			}
			FloatBuffer tmp;
			if (!file.read(fallback, tmp)) {
				out.result.addIssue("Failed to read audio file " + resolvedPath);
				return out;
			}
			out.data = SampleData(std::move(tmp));
			return out;
		}

		std::vector<uint8_t> rawBytes;
		static constexpr UInt32 chunkBytes = 32 * 1024;
		std::vector<uint8_t> chunk(chunkBytes, 0);
		AudioBufferList convertedData;
		convertedData.mNumberBuffers			  = 1;
		convertedData.mBuffers[0].mNumberChannels = audioFormat.mChannelsPerFrame;
		while (true) {
			UInt32 frames = chunkBytes / audioFormat.mBytesPerFrame;
			convertedData.mBuffers[0].mDataByteSize = chunkBytes;
			convertedData.mBuffers[0].mData			= chunk.data();
			auto err = ExtAudioFileRead(file.file, &frames, &convertedData);
			if (err != noErr) {
				out.result.addIssue("Read failed at packet for " + resolvedPath);
				return out;
			}
			if (frames == 0) break;
			const size_t bytesProduced = frames * audioFormat.mBytesPerFrame;
			rawBytes.insert(rawBytes.end(), chunk.begin(), chunk.begin() + bytesProduced);
		}

		out.data = SampleData(std::move(rawBytes), *targetFmt);
		return out;
	}
}

AudioFile::LoadedAudio AudioFile::load(const std::string &path) {
	return loadInto(path, std::nullopt);
}

AudioFile::LoadedAudio AudioFile::loadResampled(const std::string &path, int desiredSampleRate) {
	return loadInto(path, desiredSampleRate);
}
