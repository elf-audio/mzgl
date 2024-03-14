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

class DataBuffer {
public:
	explicit DataBuffer(size_t bufferSize) { data = (UInt8 *) malloc(sizeof(UInt8 *) * bufferSize); }
	~DataBuffer() {
		if (data != nullptr) {
			free(data);
		}
	}
	UInt8 *data = nullptr;
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
		auto sizePerPacket	  = audioFormat.mBytesPerPacket;
		auto packetsPerBuffer = outputBufferSize / sizePerPacket;

		DataBuffer outputBuffer {outputBufferSize};

		AudioBufferList convertedData;
		convertedData.mNumberBuffers			  = 1;
		convertedData.mBuffers[0].mNumberChannels = audioFormat.mChannelsPerFrame;
		convertedData.mBuffers[0].mDataByteSize	  = outputBufferSize;
		convertedData.mBuffers[0].mData			  = outputBuffer.data;

		bool done = false;
		while (!done) {
			auto frameCount = packetsPerBuffer;

			if (!checkStatus(ExtAudioFileRead(file, &frameCount, &convertedData), "reading packet")) {
				return false;
			}

			buff.append((float *) convertedData.mBuffers[0].mData, frameCount * audioFormat.mChannelsPerFrame);

			done = frameCount != packetsPerBuffer;
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
std::string checkItsNotAnMp4PretendingToBeAnMp3(std::string path) {
	fs::path p(path);
	if (p.extension() != ".mp3") {
		return path;
	}

	int bytesToRead = 11;
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

bool AudioFile::load(std::string path, FloatBuffer &buff, int *outNumChannels, int *outSampleRate) {
	return loadAudioFile(path, buff, std::nullopt, outNumChannels, outSampleRate);
}

template <class Buffer>
bool AudioFile_loadResampled(std::string path, Buffer &buff, int newSampleRate, int *outNumChannels) {
	return loadAudioFile(path, buff, newSampleRate, outNumChannels, nullptr);
}

bool AudioFile::loadResampled(std::string path, Int16Buffer &buff, int newSampleRate, int *outNumChannels) {
	return AudioFile_loadResampled<Int16Buffer>(path, buff, newSampleRate, outNumChannels);
}

bool AudioFile::loadResampled(std::string path, FloatBuffer &buff, int newSampleRate, int *outNumChannels) {
	return AudioFile_loadResampled<FloatBuffer>(path, buff, newSampleRate, outNumChannels);
}
