#include "tests.h"

#include "AudioFile.h"
#include "SampleData.h"
#include "SampleFormat.h"
#include "filesystem.h"

#include <cmath>
#include <string>
#include <vector>

struct AudioFileResampleFixture {
	static constexpr int sourceSampleRate	  = 44100;
	static constexpr double sourceDurationS	  = 1.0;
	static constexpr int frameTolerance		  = 4096;
	static constexpr float silentSampleFloor  = 0.05f;
	static constexpr float boundedSampleLimit = 1.5f;

	fs::path basicAudioPath(const std::string &name) const {
		return fs::current_path() / "test-files" / "basic-audio" / name;
	}

	bool anySampleExceeds(const SampleData &data, float threshold) const {
		for (size_t i = 0; i < data.size(); ++i) {
			if (std::abs(data[i]) > threshold) return true;
		}
		return false;
	}

	bool allSamplesWithin(const SampleData &data, float bound) const {
		for (size_t i = 0; i < data.size(); ++i) {
			const float v = data[i];
			if (v < -bound || v > bound) return false;
		}
		return true;
	}

	int frameCount(const AudioFile::LoadedAudio &loaded) const {
		return static_cast<int>(loaded.data.size()) / std::max(1, loaded.numChannels);
	}
};

SCENARIO_METHOD(AudioFileResampleFixture,
				"loadResampled sets sampleRate to the requested target rate",
				"[audio-file-resample]") {
	const auto path = basicAudioPath("sine440_pcm16.wav");

	for (const int targetRate : {22050, 44100, 48000, 96000}) {
		GIVEN("a 44100 Hz source loaded at target rate " + std::to_string(targetRate)) {
			auto loaded = AudioFile::loadResampled(path.string(), targetRate);

			THEN("the load succeeds") {
				REQUIRE(static_cast<bool>(loaded));
			}
			THEN("sampleRate reports the requested target rate") {
				REQUIRE(loaded.sampleRate == targetRate);
			}
		}
	}
}

SCENARIO_METHOD(AudioFileResampleFixture,
				"loadResampled frame count scales with the target sample rate",
				"[audio-file-resample]") {
	const auto path = basicAudioPath("sine440_pcm16.wav");

	for (const int targetRate : {22050, 44100, 48000, 96000}) {
		GIVEN("a 1-second 44100 Hz source loaded at target rate " + std::to_string(targetRate)) {
			auto loaded = AudioFile::loadResampled(path.string(), targetRate);
			REQUIRE(static_cast<bool>(loaded));

			THEN("the frame count is approximately targetRate * sourceDurationS") {
				const int expectedFrames = static_cast<int>(std::round(sourceDurationS * targetRate));
				REQUIRE(std::abs(frameCount(loaded) - expectedFrames) <= frameTolerance);
			}
		}
	}
}

SCENARIO_METHOD(AudioFileResampleFixture,
				"loadResampled preserves audio content across the resample",
				"[audio-file-resample]") {
	const auto path = basicAudioPath("sine440_pcm16.wav");

	for (const int targetRate : {22050, 48000, 96000}) {
		GIVEN("the 16-bit WAV loaded at target rate " + std::to_string(targetRate)) {
			auto loaded = AudioFile::loadResampled(path.string(), targetRate);
			REQUIRE(static_cast<bool>(loaded));

			THEN("data contains non-silent audio content") {
				REQUIRE(anySampleExceeds(loaded.data, silentSampleFloor));
			}
			THEN("every sample stays within a sane range") {
				REQUIRE(allSamplesWithin(loaded.data, boundedSampleLimit));
			}
		}
	}
}

SCENARIO_METHOD(AudioFileResampleFixture,
				"loadResampled preserves channel count across the resample",
				"[audio-file-resample]") {
	GIVEN("a mono source loaded at 48000 Hz") {
		auto loaded = AudioFile::loadResampled(basicAudioPath("sine440_pcm16.wav").string(), 48000);
		REQUIRE(static_cast<bool>(loaded));
		THEN("numChannels is 1") {
			REQUIRE(loaded.numChannels == 1);
		}
	}

	GIVEN("a stereo source loaded at 48000 Hz") {
		auto loaded = AudioFile::loadResampled(basicAudioPath("sine440_pcm16_stereo.wav").string(), 48000);
		REQUIRE(static_cast<bool>(loaded));
		THEN("numChannels is 2") {
			REQUIRE(loaded.numChannels == 2);
		}
		THEN("data.size() equals frames * channels") {
			REQUIRE(loaded.data.size() == static_cast<size_t>(frameCount(loaded)) * 2);
		}
	}
}

SCENARIO_METHOD(AudioFileResampleFixture,
				"Same-rate loadResampled is exact: frame count equals source frame count",
				"[audio-file-resample]") {
	GIVEN("a 1-second 44100 Hz source loaded at 44100 Hz") {
		auto loaded = AudioFile::loadResampled(basicAudioPath("sine440_pcm16.wav").string(), 44100);
		REQUIRE(static_cast<bool>(loaded));

		THEN("sampleRate is the source rate") {
			REQUIRE(loaded.sampleRate == 44100);
		}
		THEN("frame count is exactly the source frame count") {
			REQUIRE(frameCount(loaded) == 44100);
		}
		THEN("audio content survives") {
			REQUIRE(anySampleExceeds(loaded.data, silentSampleFloor));
		}
	}
}

SCENARIO_METHOD(AudioFileResampleFixture,
				"loadResampled handles multiple source formats",
				"[audio-file-resample]") {
	const std::vector<std::string> sources = {
		"sine440_pcm8.wav",
		"sine440_pcm16.wav",
		"sine440_pcm24.wav",
		"sine440_f32.wav",
		"sine440_flac16.flac",
		"sine440_flac24.flac",
		"sine440.aif",
		"sine440.mp3",
	};

	const int targetRate = 48000;

	for (const auto &fileName : sources) {
		GIVEN("the source " + fileName + " loaded at 48000 Hz") {
			auto loaded = AudioFile::loadResampled(basicAudioPath(fileName).string(), targetRate);
			REQUIRE(static_cast<bool>(loaded));

			THEN("sampleRate is 48000") {
				REQUIRE(loaded.sampleRate == targetRate);
			}
			THEN("frame count scales with the target rate") {
				const int expectedFrames = static_cast<int>(std::round(sourceDurationS * targetRate));
				REQUIRE(std::abs(frameCount(loaded) - expectedFrames) <= frameTolerance);
			}
			THEN("audio content survives") {
				REQUIRE(anySampleExceeds(loaded.data, silentSampleFloor));
			}
		}
	}
}

#ifdef __APPLE__

SCENARIO_METHOD(AudioFileResampleFixture,
				"On Apple, loadResampled preserves the source's native PCM bit depth",
				"[audio-file-resample][apple]") {
	const std::vector<std::pair<std::string, SampleFormat>> cases = {
		{"sine440_pcm8.wav", SampleFormat::I8},
		{"sine440_pcm16.wav", SampleFormat::I16},
		{"sine440_pcm24.wav", SampleFormat::I24},
		{"sine440_f32.wav", SampleFormat::F32},
	};

	for (const auto &[fileName, expectedFormat] : cases) {
		GIVEN("the source " + fileName + " loaded at 22050 Hz") {
			auto loaded = AudioFile::loadResampled(basicAudioPath(fileName).string(), 22050);
			REQUIRE(static_cast<bool>(loaded));

			THEN("the SampleData format reflects the source's bit depth, not F32") {
				REQUIRE(loaded.data.getFormat() == expectedFormat);
			}
		}
	}
}

#endif
