//
//  AudioFileDrLib.cpp
//  mzgl
//

#include "AudioFileDrLib.h"

#include "DrAudioFileReader.h"
#include "Resampler.h"
#include "SampleData.h"
#include "SampleFormat.h"

#include <cstring>
#include <filesystem>
#include <new>
#include <system_error>

namespace {
	constexpr int resamplingQuality = 5;

	void writeFloatChunkIntoSampleData(SampleData &out,
									   size_t startIdx,
									   const std::vector<float> &chunk) {
		for (size_t i = 0; i < chunk.size(); ++i) {
			out.assignValue(startIdx + i, chunk[i]);
		}
	}

	void nativeChunkToFloat(const std::vector<uint8_t> &bytes,
							SampleFormat fmt,
							std::vector<float> &outFloats) {
		const int bps  = bytesPerSample(fmt);
		const size_t n = bytes.size() / bps;
		outFloats.resize(n);
		switch (fmt) {
			case SampleFormat::I8: {
				const int8_t *p = reinterpret_cast<const int8_t *>(bytes.data());
				for (size_t i = 0; i < n; ++i) outFloats[i] = p[i] * (1.f / 128.f);
				break;
			}
			case SampleFormat::I16: {
				const int16_t *p = reinterpret_cast<const int16_t *>(bytes.data());
				for (size_t i = 0; i < n; ++i) outFloats[i] = p[i] * (1.f / 32768.f);
				break;
			}
			case SampleFormat::I24: {
				const uint8_t *p = bytes.data();
				for (size_t i = 0; i < n; ++i) {
					const uint8_t *q = p + i * 3;
					int32_t v		 = (int32_t(int8_t(q[2])) << 16) | (int32_t(q[1]) << 8) | int32_t(q[0]);
					outFloats[i]	 = v * (1.f / 8388608.f);
				}
				break;
			}
			case SampleFormat::F32: {
				const float *p = reinterpret_cast<const float *>(bytes.data());
				std::memcpy(outFloats.data(), p, n * sizeof(float));
				break;
			}
		}
	}

	void drLibTryAddIssue(AudioFile::LoadedAudio &out, const char *prefix, const std::string &path) {
		try {
			out.result.addIssue(std::string(prefix) + path);
		} catch (const std::bad_alloc &) {
			out.failureFlags |= AudioFile::LoadFailureFlag::OutOfMemory;
		} catch (...) {
		}
	}

	AudioFile::LoadedAudio drLibLoadAndCatch(const std::string &path,
											 std::optional<int> resampleTo) noexcept {
		AudioFile::LoadedAudio out;
		try {
			if (!AudioFile::drLibStreamLoad(path, out, resampleTo)) {
				out.result.addIssue("Failed to load audio file: " + path);
			}
			return out;
		} catch (const std::bad_alloc &) {
			out.failureFlags |= AudioFile::LoadFailureFlag::OutOfMemory;
			drLibTryAddIssue(out, "Out of memory loading ", path);
		} catch (const std::filesystem::filesystem_error &) {
			out.failureFlags |= AudioFile::LoadFailureFlag::FilesystemError;
			drLibTryAddIssue(out, "Filesystem error loading ", path);
		} catch (const std::system_error &) {
			out.failureFlags |= AudioFile::LoadFailureFlag::SystemError;
			drLibTryAddIssue(out, "System error loading ", path);
		} catch (const std::exception &) {
			out.failureFlags |= AudioFile::LoadFailureFlag::Exception;
			drLibTryAddIssue(out, "Exception loading ", path);
		} catch (...) {
			out.failureFlags |= AudioFile::LoadFailureFlag::Unknown;
			drLibTryAddIssue(out, "Unknown error loading ", path);
		}
		return out;
	}
} // namespace

bool AudioFile::drLibStreamLoad(const std::string &path,
								AudioFile::LoadedAudio &out,
								std::optional<int> resampleTo) {
	DrAudioFileReader reader;
	if (!reader.open(path)) {
		out.result.addIssue("Can't open file " + path);
		return false;
	}

	const int chans			= reader.getNumChannels();
	const int originalSR	= reader.getSampleRate();
	const SampleFormat fmt	= reader.getNativeFormat();
	const int targetSR		= resampleTo.value_or(originalSR);
	const bool isResampling = resampleTo.has_value() && targetSR != originalSR;

	out.numChannels = chans;
	out.sampleRate	= targetSR;
	out.data		= SampleData(std::vector<uint8_t> {}, fmt);

	Resampler resampler;
	if (isResampling) {
		resampler.init(chans, originalSR, targetSR, resamplingQuality);
	}

	constexpr size_t framesPerChunk = 1024;
	std::vector<uint8_t> nativeChunk;
	std::vector<float> floatChunk;
	std::vector<float> resampledChunk;

	while (true) {
		const uint32_t framesRead = reader.readNative(nativeChunk, framesPerChunk);
		if (framesRead == 0) break;

		if (!isResampling) {
			const size_t startBytes = out.data.byteSize();
			const size_t addBytes	= nativeChunk.size();
			const size_t totalBytes = startBytes + addBytes;
			const size_t newSamples = totalBytes / bytesPerSample(fmt);
			out.data.resize(newSamples);
			std::memcpy(out.data.bytes() + startBytes, nativeChunk.data(), addBytes);
		} else {
			nativeChunkToFloat(nativeChunk, fmt, floatChunk);
			resampler.process(floatChunk, resampledChunk);
			const size_t startIdx = out.data.size();
			out.data.resize(startIdx + resampledChunk.size());
			writeFloatChunkIntoSampleData(out.data, startIdx, resampledChunk);
		}

		if (framesRead < framesPerChunk) break;
	}

	reader.close();
	return true;
}

AudioFile::LoadedAudio AudioFile::loadViaDrLib(const std::string &path) noexcept {
	return drLibLoadAndCatch(path, std::nullopt);
}

AudioFile::LoadedAudio AudioFile::loadResampledViaDrLib(const std::string &path,
														int desiredSampleRate) noexcept {
	return drLibLoadAndCatch(path, desiredSampleRate);
}
