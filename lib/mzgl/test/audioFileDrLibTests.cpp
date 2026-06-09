#include "tests.h"

#include "AudioFile.h"
#include "SampleData.h"
#include "SampleFormat.h"
#include "filesystem.h"

#include <cmath>
#include <string>
#include <vector>

struct AudioFileDrLibFixture {
	static constexpr int sourceSampleRate	  = 44100;
	static constexpr double sourceDurationS	  = 1.0;
	static constexpr int losslessFrameTotal	  = 44100;
	static constexpr int lossyFrameTolerance  = 4096;
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

SCENARIO_METHOD(AudioFileDrLibFixture,
				"loadViaDrLib succeeds on a 16-bit mono WAV",
				"[audio-file-load-drlib]") {
	GIVEN("the path to sine440_pcm16.wav") {
		const auto path = basicAudioPath("sine440_pcm16.wav");

		WHEN("AudioFile::loadViaDrLib is called") {
			auto loaded = AudioFile::loadViaDrLib(path.string());

			THEN("the LoadedAudio reports success") {
				REQUIRE(static_cast<bool>(loaded));
				REQUIRE(loaded.result.success());
				REQUIRE(loaded.failureFlags == AudioFile::LoadFailureFlag::None);
			}
			THEN("the channel count is 1") { REQUIRE(loaded.numChannels == 1); }
			THEN("the sample rate is the source rate") {
				REQUIRE(loaded.sampleRate == sourceSampleRate);
			}
			THEN("the frame count matches a 1-second mono buffer") {
				REQUIRE(frameCount(loaded) == losslessFrameTotal);
			}
			THEN("the data contains audio content") {
				REQUIRE(anySampleExceeds(loaded.data, silentSampleFloor));
			}
			THEN("every sample is within a sane range") {
				REQUIRE(allSamplesWithin(loaded.data, boundedSampleLimit));
			}
		}
	}
}

SCENARIO_METHOD(AudioFileDrLibFixture,
				"loadViaDrLib preserves stereo channel count",
				"[audio-file-load-drlib]") {
	GIVEN("the path to sine440_pcm16_stereo.wav") {
		auto loaded = AudioFile::loadViaDrLib(basicAudioPath("sine440_pcm16_stereo.wav").string());

		THEN("the load succeeds") { REQUIRE(static_cast<bool>(loaded)); }
		THEN("two channels are reported") { REQUIRE(loaded.numChannels == 2); }
		THEN("the frame count matches a 1-second stereo buffer") {
			REQUIRE(frameCount(loaded) == losslessFrameTotal);
		}
		THEN("data.size() equals frames * channels") {
			REQUIRE(loaded.data.size() == static_cast<size_t>(losslessFrameTotal) * 2);
		}
	}
}

SCENARIO_METHOD(AudioFileDrLibFixture,
				"loadViaDrLib handles every lossless PCM bit depth in WAV",
				"[audio-file-load-drlib]") {
	for (const auto &fileName : std::vector<std::string> {
			 "sine440_pcm8.wav",
			 "sine440_pcm16.wav",
			 "sine440_pcm24.wav",
			 "sine440_f32.wav",
		 }) {
		GIVEN("the path to " + fileName) {
			auto loaded = AudioFile::loadViaDrLib(basicAudioPath(fileName).string());

			THEN("the load succeeds") { REQUIRE(static_cast<bool>(loaded)); }
			THEN("channel count is 1") { REQUIRE(loaded.numChannels == 1); }
			THEN("sample rate is the source rate") {
				REQUIRE(loaded.sampleRate == sourceSampleRate);
			}
			THEN("frame count is exactly 1 second of mono audio") {
				REQUIRE(frameCount(loaded) == losslessFrameTotal);
			}
			THEN("audio content is present") {
				REQUIRE(anySampleExceeds(loaded.data, silentSampleFloor));
			}
			THEN("samples stay within a sane range") {
				REQUIRE(allSamplesWithin(loaded.data, boundedSampleLimit));
			}
		}
	}
}

SCENARIO_METHOD(AudioFileDrLibFixture,
				"loadViaDrLib handles FLAC at multiple bit depths",
				"[audio-file-load-drlib]") {
	for (const auto &fileName : std::vector<std::string> {"sine440_flac16.flac", "sine440_flac24.flac"}) {
		GIVEN("the path to " + fileName) {
			auto loaded = AudioFile::loadViaDrLib(basicAudioPath(fileName).string());

			THEN("the load succeeds") { REQUIRE(static_cast<bool>(loaded)); }
			THEN("channel count is 1") { REQUIRE(loaded.numChannels == 1); }
			THEN("sample rate is the source rate") {
				REQUIRE(loaded.sampleRate == sourceSampleRate);
			}
			THEN("frame count is exactly 1 second of mono audio") {
				REQUIRE(frameCount(loaded) == losslessFrameTotal);
			}
			THEN("audio content is present") {
				REQUIRE(anySampleExceeds(loaded.data, silentSampleFloor));
			}
		}
	}
}

SCENARIO_METHOD(AudioFileDrLibFixture,
				"loadViaDrLib handles AIFF",
				"[audio-file-load-drlib]") {
	auto loaded = AudioFile::loadViaDrLib(basicAudioPath("sine440.aif").string());

	THEN("the load succeeds") { REQUIRE(static_cast<bool>(loaded)); }
	THEN("channel count is 1") { REQUIRE(loaded.numChannels == 1); }
	THEN("sample rate is the source rate") { REQUIRE(loaded.sampleRate == sourceSampleRate); }
	THEN("frame count is exactly 1 second of mono audio") {
		REQUIRE(frameCount(loaded) == losslessFrameTotal);
	}
	THEN("audio content is present") {
		REQUIRE(anySampleExceeds(loaded.data, silentSampleFloor));
	}
}

SCENARIO_METHOD(AudioFileDrLibFixture, "loadViaDrLib handles MP3", "[audio-file-load-drlib]") {
	auto loaded = AudioFile::loadViaDrLib(basicAudioPath("sine440.mp3").string());

	THEN("the load succeeds") { REQUIRE(static_cast<bool>(loaded)); }
	THEN("channel count is 1") { REQUIRE(loaded.numChannels == 1); }
	THEN("sample rate is the source rate") { REQUIRE(loaded.sampleRate == sourceSampleRate); }
	THEN("frame count is within encoder-delay tolerance of 1 second") {
		REQUIRE(std::abs(frameCount(loaded) - losslessFrameTotal) <= lossyFrameTolerance);
	}
	THEN("audio content is present") {
		REQUIRE(anySampleExceeds(loaded.data, silentSampleFloor));
	}
}

SCENARIO_METHOD(AudioFileDrLibFixture,
				"loadViaDrLib on .m4a (AAC/ALAC) follows platform codec support",
				"[audio-file-load-drlib]") {
	// DrAudioFileReader is not dr_libs-only: on Windows it hands m4a/aac
	// containers to Media Foundation, which decodes AAC and ALAC. On other
	// platforms dr_libs has no decoder for them and there is no fallback, so
	// the open fails. Assert the real per-platform contract rather than a
	// single hard-coded outcome.
	for (const auto &fileName : std::vector<std::string> {"sine440_aac.m4a", "sine440_alac.m4a"}) {
		GIVEN("the path to " + fileName) {
			auto loaded = AudioFile::loadViaDrLib(basicAudioPath(fileName).string());

#ifdef _WIN32
			// On Windows m4a/aac/alac is decoded by Media Foundation.
			THEN("the load succeeds (decoded via Media Foundation)") {
				REQUIRE(static_cast<bool>(loaded));
			}
			THEN("channel count is 1") { REQUIRE(loaded.numChannels == 1); }
			THEN("sample rate is the source rate") {
				REQUIRE(loaded.sampleRate == sourceSampleRate);
			}
			THEN("the data contains audio content") {
				REQUIRE(anySampleExceeds(loaded.data, silentSampleFloor));
			}
			THEN("lossy m4a is decoded to I16 (bounded memory, matches other platforms)") {
				REQUIRE(loaded.data.getFormat() == SampleFormat::I16);
			}
#else
			THEN("the load reports failure (no AAC/ALAC decoder available)") {
				REQUIRE_FALSE(static_cast<bool>(loaded));
			}
			THEN("at least one issue is recorded") {
				REQUIRE_FALSE(loaded.result.issues.empty());
			}
#endif
		}
	}
}

SCENARIO_METHOD(AudioFileDrLibFixture,
				"loadResampledViaDrLib returns the requested sample rate",
				"[audio-file-load-drlib]") {
	GIVEN("a 16-bit WAV and a target rate of 22050 Hz") {
		auto loaded = AudioFile::loadResampledViaDrLib(basicAudioPath("sine440_pcm16.wav").string(), 22050);

		THEN("the load succeeds") { REQUIRE(static_cast<bool>(loaded)); }
		THEN("channel count is preserved") { REQUIRE(loaded.numChannels == 1); }
		THEN("sample rate is the requested target rate") { REQUIRE(loaded.sampleRate == 22050); }
		THEN("frame count scales with the target sample rate") {
			const int expectedFrames = static_cast<int>(std::round(sourceDurationS * 22050));
			REQUIRE(std::abs(frameCount(loaded) - expectedFrames) <= lossyFrameTolerance);
		}
		THEN("audio content survives the resample") {
			REQUIRE(anySampleExceeds(loaded.data, silentSampleFloor));
		}
	}
}

SCENARIO_METHOD(AudioFileDrLibFixture,
				"loadResampledViaDrLib upsamples as well as downsamples",
				"[audio-file-load-drlib]") {
	auto loaded = AudioFile::loadResampledViaDrLib(basicAudioPath("sine440_pcm16.wav").string(), 96000);

	THEN("the load succeeds") { REQUIRE(static_cast<bool>(loaded)); }
	THEN("sample rate is the requested target rate") { REQUIRE(loaded.sampleRate == 96000); }
	THEN("frame count scales with the target sample rate") {
		const int expectedFrames = static_cast<int>(std::round(sourceDurationS * 96000));
		REQUIRE(std::abs(frameCount(loaded) - expectedFrames) <= lossyFrameTolerance);
	}
}

SCENARIO_METHOD(AudioFileDrLibFixture,
				"loadResampledViaDrLib preserves stereo channel count",
				"[audio-file-load-drlib]") {
	auto loaded =
		AudioFile::loadResampledViaDrLib(basicAudioPath("sine440_pcm16_stereo.wav").string(), 48000);

	THEN("the load succeeds") { REQUIRE(static_cast<bool>(loaded)); }
	THEN("channel count remains 2") { REQUIRE(loaded.numChannels == 2); }
	THEN("sample rate is the requested target rate") { REQUIRE(loaded.sampleRate == 48000); }
	THEN("data size equals frames * channels") {
		REQUIRE(loaded.data.size() == static_cast<size_t>(frameCount(loaded)) * 2);
	}
}

SCENARIO_METHOD(AudioFileDrLibFixture,
				"dr_libs PCM-WAV loads retain on-disk bit depth",
				"[audio-file-load-drlib]") {
	const std::vector<std::pair<std::string, SampleFormat>> cases = {
		{"sine440_pcm8.wav", SampleFormat::I8},
		{"sine440_pcm16.wav", SampleFormat::I16},
		{"sine440_pcm24.wav", SampleFormat::I24},
		{"sine440_f32.wav", SampleFormat::F32},
	};

	for (const auto &[fileName, expectedFormat] : cases) {
		GIVEN("the path to " + fileName) {
			auto loaded = AudioFile::loadViaDrLib(basicAudioPath(fileName).string());
			THEN("the load succeeds and the SampleData format matches the source depth") {
				REQUIRE(static_cast<bool>(loaded));
				REQUIRE(loaded.data.getFormat() == expectedFormat);
			}
		}
	}
}

SCENARIO_METHOD(AudioFileDrLibFixture,
				"dr_libs FLAC loads retain native depth from the FLAC header",
				"[audio-file-load-drlib]") {
	const std::vector<std::pair<std::string, SampleFormat>> cases = {
		{"sine440_flac16.flac", SampleFormat::I16},
		{"sine440_flac24.flac", SampleFormat::I24},
	};

	for (const auto &[fileName, expectedFormat] : cases) {
		GIVEN("the path to " + fileName) {
			auto loaded = AudioFile::loadViaDrLib(basicAudioPath(fileName).string());
			THEN("the SampleData format reflects the FLAC header depth") {
				REQUIRE(static_cast<bool>(loaded));
				REQUIRE(loaded.data.getFormat() == expectedFormat);
			}
		}
	}
}

SCENARIO_METHOD(AudioFileDrLibFixture,
				"dr_libs MP3 loads produce I16",
				"[audio-file-load-drlib]") {
	auto loaded = AudioFile::loadViaDrLib(basicAudioPath("sine440.mp3").string());

	THEN("the load succeeds and the SampleData format is I16") {
		REQUIRE(static_cast<bool>(loaded));
		REQUIRE(loaded.data.getFormat() == SampleFormat::I16);
	}
}

SCENARIO_METHOD(AudioFileDrLibFixture,
				"dr_libs AIFF loads produce F32",
				"[audio-file-load-drlib]") {
	auto loaded = AudioFile::loadViaDrLib(basicAudioPath("sine440.aif").string());

	THEN("the load succeeds and the SampleData format is F32") {
		REQUIRE(static_cast<bool>(loaded));
		REQUIRE(loaded.data.getFormat() == SampleFormat::F32);
	}
}

SCENARIO_METHOD(AudioFileDrLibFixture,
				"loadViaDrLib reports failure for a non-existent file without throwing",
				"[audio-file-load-drlib]") {
	const auto path = fs::current_path() / "test-files" / "this-file-does-not-exist.wav";
	REQUIRE_FALSE(fs::exists(path));

	AudioFile::LoadedAudio loaded;
	REQUIRE_NOTHROW(loaded = AudioFile::loadViaDrLib(path.string()));

	THEN("the load reports failure") { REQUIRE_FALSE(static_cast<bool>(loaded)); }
	THEN("at least one issue is recorded") {
		REQUIRE_FALSE(loaded.result.issues.empty());
	}
}
