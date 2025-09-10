//
//  AudioFile.cpp
//  MZGL
//
//  Created by Marek Bereza on 01/03/2019.
//  Copyright © 2019 Marek Bereza. All rights reserved.
//

#include "AudioFile.h"
#ifdef __ANDROID__
#	include "androidUtil.h"
#	include "media/NdkMediaExtractor.h"
#	include "media/NdkMediaFormat.h"
#endif

#include "filesystem.h"
#include "util.h"
#include "util/log.h"
#include "log.h"

#include "Resampler.h"
#include "DrAudioFileReader.h"

using namespace std;

#define RESAMPLING_QUALITY 5
bool AudioFile_loadDrLib(std::string path,
						 FloatBuffer &buff,
						 int *outNumChannels,
						 int *outSampleRate,
						 const int desiredSampleRate = 0) {
	Resampler resampler;
	bool isResampling = false;

	DrAudioFileReader inFile;
	if (!inFile.open(path)) {
		Log::e() << "Can't open file " << path << " for reading";
		return false;
	}

	*outNumChannels		   = inFile.getNumChannels();
	int originalSampleRate = inFile.getSampleRate();

	if (desiredSampleRate != 0) {
		if (desiredSampleRate != originalSampleRate) {
			isResampling = true;
			Log::d() << "Resampling from " << originalSampleRate << "Hz to " << desiredSampleRate << "Hz";
			resampler.init(inFile.getNumChannels(), inFile.getSampleRate(), desiredSampleRate, RESAMPLING_QUALITY);
		}
		*outSampleRate = desiredSampleRate;
	} else {
		*outSampleRate = originalSampleRate;
	}

	int buffSize = 1024;
	vector<float> inBuff(buffSize);
	while (1) {
		int framesRead = inFile.read(inBuff);

		// obvs only convert if you need to!
		if (isResampling) {
			vector<float> resampled;

			resampler.process(inBuff, resampled);
			buff.insert(buff.end(), resampled.begin(), resampled.end());
		} else {
			buff.insert(buff.end(), inBuff.begin(), inBuff.end());
		}

		//printf("frames read: %d\n", framesRead);
		if (framesRead != buffSize / inFile.getNumChannels()) break;
	}

	return true;
}
#ifdef __ANDROID__

int getFirstAudioTrackId(AMediaExtractor *extractor) {
	int numTracks = AMediaExtractor_getTrackCount(extractor);
	for (int i = 0; i < numTracks; i++) {
		AMediaFormat *format = AMediaExtractor_getTrackFormat(extractor, i);
		const char *mimeType;
		if (AMediaFormat_getString(format, AMEDIAFORMAT_KEY_MIME, &mimeType)) {
			if (strstr(mimeType, "audio/") != nullptr) {
				return i;
			}
		}
	}
	return -1;
}
#endif

#ifdef __ANDROID__
// if desiredSampleRate is zero, no resampling is done
bool AudioFileAndroid_load(std::string path,
						   FloatBuffer &buff,
						   int *outNumChannels,
						   int *outSampleRate,
						   const int desiredSampleRate = 0) {
	Log::d() << "Using NDK decoder";

	// open asset as file descriptor
	off_t start;

	FILE *fp = fopen(path.c_str(), "rb");

	if (fp == nullptr) {
		Log::e() << "Can't open file, maybe doesn't exist (path: " << path << ")";
		return false;
	}

	fseek(fp, 0, SEEK_END);
	long size = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	// Extract the audio frames
	AMediaExtractor *extractor = AMediaExtractor_new();
	auto amresult			   = AMediaExtractor_setDataSourceFd(extractor, fileno(fp), 0, size);

	if (amresult != AMEDIA_OK) {
		Log::e() << "Error setting extractor data source, err " << amresult;
		AMediaExtractor_delete(extractor);
		return false;
	}

	int firstAudioTrackId = getFirstAudioTrackId(extractor);

	if (firstAudioTrackId == -1) {
		Log::e() << "No audio track found in file, trying with track 0 (will probs fail)";
		firstAudioTrackId = 0;
		return false;
	}
	// Specify our desired output format by creating it from our source
	AMediaFormat *format = AMediaExtractor_getTrackFormat(extractor, firstAudioTrackId);

	int32_t originalSampleRate;
	if (AMediaFormat_getInt32(format, AMEDIAFORMAT_KEY_SAMPLE_RATE, &originalSampleRate)) {
		Log::d() << "Source sample rate " << originalSampleRate;
	} else {
		Log::e() << "Failed to get sample rate";
		AMediaExtractor_delete(extractor);
		AMediaFormat_delete(format);
		return false;
	}

	int32_t fileChannelCount;
	if (AMediaFormat_getInt32(format, AMEDIAFORMAT_KEY_CHANNEL_COUNT, &fileChannelCount)) {
		Log::d() << "Got channel count " << fileChannelCount;
		*outNumChannels = fileChannelCount;
		if (fileChannelCount != 1 && fileChannelCount != 2) {
			Log::d() << "Sample not mono or stereo -" << fileChannelCount << " channels";
			AMediaExtractor_delete(extractor);
			AMediaFormat_delete(format);
			return 0;
		}
	} else {
		Log::e() << "Failed to get channel count";
		AMediaExtractor_delete(extractor);
		AMediaFormat_delete(format);
		return 0;
	}

	const char *formatStr = AMediaFormat_toString(format);
	Log::d() << "Output format " << formatStr;

	const char *mimeType;
	if (AMediaFormat_getString(format, AMEDIAFORMAT_KEY_MIME, &mimeType)) {
		Log::d() << "Got mime type " << mimeType;
	} else {
		Log::e() << "Failed to get mime type";
		AMediaExtractor_delete(extractor);
		AMediaFormat_delete(format);
		return 0;
	}

	// Obtain the correct decoder
	AMediaCodec *codec = nullptr;
	AMediaExtractor_selectTrack(extractor, firstAudioTrackId);
	codec = AMediaCodec_createDecoderByType(mimeType);
	AMediaCodec_configure(codec, format, nullptr, nullptr, 0);
	AMediaCodec_start(codec);

	// DECODE

	bool isExtracting = true;
	bool isDecoding	  = true;
	// int32_t bytesWritten = 0;

	Resampler resampler;
	vector<float> resampled;
	bool isResampling = false;

	auto configureResampler = [&](int inRate) {
		if (desiredSampleRate == 0) {
			*outSampleRate = inRate;
			isResampling   = false;
			return;
		}
		*outSampleRate = desiredSampleRate;
		if (desiredSampleRate == inRate) {
			isResampling = false;
		} else {
			isResampling = true;
			resampler.init(fileChannelCount, inRate, desiredSampleRate, RESAMPLING_QUALITY);
		}
	};

	configureResampler(originalSampleRate);

	constexpr float sint16ToFloat = (1.0f / 32768.0f);
	int decoderChannelCount		  = fileChannelCount;
	while (isExtracting || isDecoding) {
		if (isExtracting) {
			// Obtain the index of the next available input buffer
			ssize_t inputIndex = AMediaCodec_dequeueInputBuffer(codec, 2000);
			//LOGV("Got input buffer %d", inputIndex);

			// The input index acts as a status if its negative
			if (inputIndex < 0) {
				if (inputIndex == AMEDIACODEC_INFO_TRY_AGAIN_LATER) {
					// LOGV("Codec.dequeueInputBuffer try again later");
				} else {
					Log::d() << "Codec.dequeueInputBuffer unknown error status";
				}
			} else {
				// Obtain the actual buffer and read the encoded data into it
				size_t inputSize;
				uint8_t *inputBuffer = AMediaCodec_getInputBuffer(codec, inputIndex, &inputSize);
				//LOGV("Sample size is: %d", inputSize);

				ssize_t sampleSize		= AMediaExtractor_readSampleData(extractor, inputBuffer, inputSize);
				auto presentationTimeUs = AMediaExtractor_getSampleTime(extractor);

				if (sampleSize > 0) {
					// Enqueue the encoded data
					AMediaCodec_queueInputBuffer(codec, inputIndex, 0, sampleSize, presentationTimeUs, 0);
					AMediaExtractor_advance(extractor);

				} else {
					Log::d() << "End of extractor data stream";
					isExtracting = false;

					// We need to tell the codec that we've reached the end of the stream
					AMediaCodec_queueInputBuffer(
						codec, inputIndex, 0, 0, presentationTimeUs, AMEDIACODEC_BUFFER_FLAG_END_OF_STREAM);
				}
			}
		}

		if (isDecoding) {
			// Dequeue the decoded data
			AMediaCodecBufferInfo info;
			ssize_t outputIndex = AMediaCodec_dequeueOutputBuffer(codec, &info, 0);

			if (outputIndex >= 0) {
				// Check whether this is set earlier
				if (info.flags & AMEDIACODEC_BUFFER_FLAG_END_OF_STREAM) {
					Log::d() << "Reached end of decoding stream";
					isDecoding = false;
				} else {
					// Valid index, acquire buffer
					size_t outputSize;
					uint8_t *outputBuffer = AMediaCodec_getOutputBuffer(codec, outputIndex, &outputSize);

					// copy the data out of the buffer - can only  be uint16_t with NdkMediaDecoder
					int16_t *outBuff = (int16_t *) outputBuffer;
					int numSamples	 = info.size / 2;

					int sampleAdvance = 1;
					if (fileChannelCount == 1 && decoderChannelCount == 2) {
						// if the file is mono but the decoder is outputting stereo, we need to halve the number of samples
						numSamples /= 2;
						sampleAdvance = 2;
					}
					vector<float> fBuff(numSamples);
					for (int i = 0; i < numSamples; i++)
						fBuff[i] = (float) outBuff[i * sampleAdvance] * sint16ToFloat;

					if (isResampling) {
						resampler.process(fBuff, resampled);
						buff.insert(buff.end(), resampled.begin(), resampled.end());
					} else {
						buff.insert(buff.end(), fBuff.begin(), fBuff.end());
					}
					//bytesWritten+=info.size;
					AMediaCodec_releaseOutputBuffer(codec, outputIndex, false);
				}

			} else {
				// The outputIndex doubles as a status return if its value is < 0
				switch (outputIndex) {
					case AMEDIACODEC_INFO_TRY_AGAIN_LATER: LOGD("dequeueOutputBuffer: try again later"); break;
					case AMEDIACODEC_INFO_OUTPUT_BUFFERS_CHANGED:
						Log::d() << "dequeueOutputBuffer: output buffers changed";
						break;
					case AMEDIACODEC_INFO_OUTPUT_FORMAT_CHANGED:
						Log::d() << "dequeueOutputBuffer: output outputFormat changed";
						format = AMediaCodec_getOutputFormat(codec);
						Log::d() << "outputFormat changed to: " << AMediaFormat_toString(format);
						int32_t newRate	 = 0;
						int32_t newChans = 0;
						if (AMediaFormat_getInt32(format, AMEDIAFORMAT_KEY_CHANNEL_COUNT, &newChans))
							decoderChannelCount = newChans;

						if (AMediaFormat_getInt32(format, AMEDIAFORMAT_KEY_SAMPLE_RATE, &newRate))
							originalSampleRate = newRate;

						configureResampler(originalSampleRate);
						Log::d() << "Reconfiguring starting sample rate to " << originalSampleRate;

						break;
				}
			}
		}
	}

	// Clean up
	AMediaFormat_delete(format);
	AMediaCodec_delete(codec);
	AMediaExtractor_delete(extractor);

	return true;
}
#endif

bool AudioFile::load(std::string path, FloatBuffer &buff, int *outNumChannels, int *outSampleRate) {
#ifdef __ANDROID__
	// on some shit android phones, the OS level audio file loading is broken somehow
	// so if you're loading a wav file, better to stick with dr_wav
	if (fs::path(toLowerCase(path)).extension() == ".wav") {
		return AudioFile_loadDrLib(path, buff, outNumChannels, outSampleRate, 0);
	}
	return AudioFileAndroid_load(path, buff, outNumChannels, outSampleRate, 0);
#else
	return AudioFile_loadDrLib(path, buff, outNumChannels, outSampleRate, 0);
#endif
}

bool AudioFile::loadResampled(std::string path, FloatBuffer &buff, int newSampleRate, int *outNumChannels) {
#ifdef __ANDROID__
	string ext = toLowerCase(fs::path(path).extension().string());
	// this is an issue because people are loading .mp3 extensions that are actually m4a's from youtube.
	// check to see if Dr Lib fails on AAC and if it does, send it to NDK decoder and see if that works.
	// (e.g. do a test with an AAC renamed to MP3

	if (ext == ".wav" || ext == ".flac" || ext == ".mp3") {
		// if it's a wav file, use dr wav
		Log::d() << "USING drlib decoding " << path;
		int outSampleRate	= 0;
		bool successOpening = AudioFile_loadDrLib(path, buff, outNumChannels, &outSampleRate, newSampleRate);
		if (successOpening) return true;
	}

	// fallback to ndk decoder if nothing else can open it.
	Log::d() << "USING ndk media decoder to open " << path;
	int outSampleRate = 0;

	bool success = AudioFileAndroid_load(path, buff, outNumChannels, &outSampleRate, newSampleRate);
	Log::d() << "Loaded " << path << " with ndk decoder, success: " << success << ", "
			 << "sample rate: " << outSampleRate << ", channels: " << (outNumChannels ? *outNumChannels : -1)
			 << ", samples: " << buff.size();
	Log::d() << "Resampled to " << newSampleRate << " hz";
	return success;

#else
	int outSampleRate = 0;
	return AudioFile_loadDrLib(path, buff, outNumChannels, &outSampleRate, newSampleRate);
#endif
}
