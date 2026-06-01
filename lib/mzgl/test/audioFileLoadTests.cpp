#include "tests.h"

#include "AudioFile.h"
#include "SampleData.h"
#include "SampleFormat.h"
#include "filesystem.h"

#include <cmath>
#include <string>
#include <vector>

struct AudioFileLoadFixture {
	static constexpr int sourceSampleRate	  = 44100;
	static constexpr double sourceDurationS	  = 1.0;
	static constexpr float sourceFrequencyHz  = 440.0f;
	static constexpr float sourceAmplitude	  = 0.5f;
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

SCENARIO_METHOD(AudioFileLoadFixture,
				"AudioFile::load returns a successful LoadedAudio for a 16-bit mono WAV",
				"[audio-file-load]") {
	GIVEN("the path to sine440_pcm16.wav") {
		const auto path = basicAudioPath("sine440_pcm16.wav");

		WHEN("AudioFile::load is called") {
			auto loaded = AudioFile::load(path.string());

			THEN("the LoadedAudio reports success") {
				REQUIRE(static_cast<bool>(loaded));
				REQUIRE(loaded.result.success());
			}
			THEN("the channel count matches the source") {
				REQUIRE(loaded.numChannels == 1);
			}
			THEN("the sample rate matches the source") {
				REQUIRE(loaded.sampleRate == sourceSampleRate);
			}
			THEN("the frame count matches a 1-second mono buffer") {
				REQUIRE(frameCount(loaded) == losslessFrameTotal);
			}
			THEN("the data contains audio content") {
				REQUIRE(anySampleExceeds(loaded.data, silentSampleFloor));
			}
			THEN("every sample is within a sane amplitude range") {
				REQUIRE(allSamplesWithin(loaded.data, boundedSampleLimit));
			}
		}
	}
}

SCENARIO_METHOD(AudioFileLoadFixture,
				"AudioFile::load reports the source channel count for a stereo WAV",
				"[audio-file-load]") {
	GIVEN("the path to sine440_pcm16_stereo.wav") {
		const auto path = basicAudioPath("sine440_pcm16_stereo.wav");

		WHEN("AudioFile::load is called") {
			auto loaded = AudioFile::load(path.string());

			THEN("the load succeeds") { REQUIRE(static_cast<bool>(loaded)); }
			THEN("two channels are reported") { REQUIRE(loaded.numChannels == 2); }
			THEN("the sample rate is the source rate") {
				REQUIRE(loaded.sampleRate == sourceSampleRate);
			}
			THEN("the frame count matches a 1-second stereo buffer") {
				REQUIRE(frameCount(loaded) == losslessFrameTotal);
			}
			THEN("data.size() equals frames * channels") {
				REQUIRE(loaded.data.size() == static_cast<size_t>(losslessFrameTotal) * 2);
			}
			THEN("the data contains audio content") {
				REQUIRE(anySampleExceeds(loaded.data, silentSampleFloor));
			}
		}
	}
}

SCENARIO_METHOD(AudioFileLoadFixture,
				"AudioFile::load handles every lossless PCM bit depth in WAV",
				"[audio-file-load]") {
	for (const auto &fileName : std::vector<std::string> {
			 "sine440_pcm8.wav",
			 "sine440_pcm16.wav",
			 "sine440_pcm24.wav",
			 "sine440_f32.wav",
		 }) {
		GIVEN("the path to " + fileName) {
			const auto path = basicAudioPath(fileName);

			WHEN("AudioFile::load is called") {
				auto loaded = AudioFile::load(path.string());

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
}

SCENARIO_METHOD(AudioFileLoadFixture,
				"AudioFile::load handles FLAC at multiple bit depths",
				"[audio-file-load]") {
	for (const auto &fileName : std::vector<std::string> {"sine440_flac16.flac", "sine440_flac24.flac"}) {
		GIVEN("the path to " + fileName) {
			const auto path = basicAudioPath(fileName);

			WHEN("AudioFile::load is called") {
				auto loaded = AudioFile::load(path.string());

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
}

SCENARIO_METHOD(AudioFileLoadFixture,
				"AudioFile::load handles AIFF",
				"[audio-file-load]") {
	GIVEN("the path to sine440.aif") {
		const auto path = basicAudioPath("sine440.aif");

		WHEN("AudioFile::load is called") {
			auto loaded = AudioFile::load(path.string());

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

SCENARIO_METHOD(AudioFileLoadFixture,
				"AudioFile::load handles ALAC inside an .m4a container",
				"[audio-file-load]") {
	GIVEN("the path to sine440_alac.m4a") {
		const auto path = basicAudioPath("sine440_alac.m4a");

		WHEN("AudioFile::load is called") {
			auto loaded = AudioFile::load(path.string());

			THEN("the load succeeds") { REQUIRE(static_cast<bool>(loaded)); }
			THEN("channel count is 1") { REQUIRE(loaded.numChannels == 1); }
			THEN("sample rate is the source rate") {
				REQUIRE(loaded.sampleRate == sourceSampleRate);
			}
			THEN("frame count is within encoder-delay tolerance of 1 second") {
				REQUIRE(std::abs(frameCount(loaded) - losslessFrameTotal) <= lossyFrameTolerance);
			}
			THEN("audio content is present") {
				REQUIRE(anySampleExceeds(loaded.data, silentSampleFloor));
			}
		}
	}
}

SCENARIO_METHOD(AudioFileLoadFixture,
				"AudioFile::load handles lossy MP3 and AAC inputs",
				"[audio-file-load]") {
	for (const auto &fileName : std::vector<std::string> {"sine440.mp3", "sine440_aac.m4a"}) {
		GIVEN("the path to " + fileName) {
			const auto path = basicAudioPath(fileName);

			WHEN("AudioFile::load is called") {
				auto loaded = AudioFile::load(path.string());

				THEN("the load succeeds") { REQUIRE(static_cast<bool>(loaded)); }
				THEN("channel count is 1") { REQUIRE(loaded.numChannels == 1); }
				THEN("sample rate is the source rate") {
					REQUIRE(loaded.sampleRate == sourceSampleRate);
				}
				THEN("frame count is within encoder-delay tolerance of 1 second") {
					REQUIRE(std::abs(frameCount(loaded) - losslessFrameTotal) <= lossyFrameTolerance);
				}
				THEN("audio content is present") {
					REQUIRE(anySampleExceeds(loaded.data, silentSampleFloor));
				}
			}
		}
	}
}

SCENARIO_METHOD(AudioFileLoadFixture,
				"AudioFile::loadResampled returns the requested sample rate",
				"[audio-file-load]") {
	GIVEN("the path to sine440_pcm16.wav and a target rate of 22050 Hz") {
		const auto path			 = basicAudioPath("sine440_pcm16.wav");
		const int targetSampleRate = 22050;

		WHEN("AudioFile::loadResampled is called") {
			auto loaded = AudioFile::loadResampled(path.string(), targetSampleRate);

			THEN("the load succeeds") { REQUIRE(static_cast<bool>(loaded)); }
			THEN("channel count is preserved") { REQUIRE(loaded.numChannels == 1); }
			THEN("sample rate is the requested target rate") {
				REQUIRE(loaded.sampleRate == targetSampleRate);
			}
			THEN("frame count scales with the target sample rate") {
				const int expectedFrames =
					static_cast<int>(std::round(sourceDurationS * targetSampleRate));
				REQUIRE(std::abs(frameCount(loaded) - expectedFrames) <= lossyFrameTolerance);
			}
			THEN("audio content survives the resample") {
				REQUIRE(anySampleExceeds(loaded.data, silentSampleFloor));
			}
		}
	}
}

SCENARIO_METHOD(AudioFileLoadFixture,
				"AudioFile::loadResampled upsamples as well as downsamples",
				"[audio-file-load]") {
	GIVEN("the path to sine440_pcm16.wav and a target rate of 96000 Hz") {
		const auto path			 = basicAudioPath("sine440_pcm16.wav");
		const int targetSampleRate = 96000;

		WHEN("AudioFile::loadResampled is called") {
			auto loaded = AudioFile::loadResampled(path.string(), targetSampleRate);

			THEN("the load succeeds") { REQUIRE(static_cast<bool>(loaded)); }
			THEN("sample rate is the requested target rate") {
				REQUIRE(loaded.sampleRate == targetSampleRate);
			}
			THEN("frame count scales with the target sample rate") {
				const int expectedFrames =
					static_cast<int>(std::round(sourceDurationS * targetSampleRate));
				REQUIRE(std::abs(frameCount(loaded) - expectedFrames) <= lossyFrameTolerance);
			}
			THEN("audio content survives the resample") {
				REQUIRE(anySampleExceeds(loaded.data, silentSampleFloor));
			}
		}
	}
}

SCENARIO_METHOD(AudioFileLoadFixture,
				"AudioFile::loadResampled preserves stereo channel count",
				"[audio-file-load]") {
	GIVEN("the path to sine440_pcm16_stereo.wav and a target rate of 48000 Hz") {
		const auto path			 = basicAudioPath("sine440_pcm16_stereo.wav");
		const int targetSampleRate = 48000;

		WHEN("AudioFile::loadResampled is called") {
			auto loaded = AudioFile::loadResampled(path.string(), targetSampleRate);

			THEN("the load succeeds") { REQUIRE(static_cast<bool>(loaded)); }
			THEN("channel count remains 2") { REQUIRE(loaded.numChannels == 2); }
			THEN("sample rate is the requested target rate") {
				REQUIRE(loaded.sampleRate == targetSampleRate);
			}
			THEN("data size equals frames * channels") {
				REQUIRE(loaded.data.size() == static_cast<size_t>(frameCount(loaded)) * 2);
			}
		}
	}
}

#ifdef __APPLE__

SCENARIO_METHOD(AudioFileLoadFixture,
				"On Apple, AudioFile::load retains the source PCM bit depth in the SampleData",
				"[audio-file-load][apple]") {
	const std::vector<std::pair<std::string, SampleFormat>> cases = {
		{"sine440_pcm8.wav", SampleFormat::I8},
		{"sine440_pcm16.wav", SampleFormat::I16},
		{"sine440_pcm24.wav", SampleFormat::I24},
		{"sine440_f32.wav", SampleFormat::F32},
	};

	for (const auto &[fileName, expectedFormat] : cases) {
		GIVEN("the path to " + fileName) {
			auto loaded = AudioFile::load(basicAudioPath(fileName).string());

			THEN("the load succeeds and the SampleData format matches the source depth") {
				REQUIRE(static_cast<bool>(loaded));
				REQUIRE(loaded.data.getFormat() == expectedFormat);
			}
		}
	}
}

SCENARIO_METHOD(AudioFileLoadFixture,
				"On Apple, AudioFile::load retains FLAC native depth from the FLAC header",
				"[audio-file-load][apple]") {
	const std::vector<std::pair<std::string, SampleFormat>> cases = {
		{"sine440_flac16.flac", SampleFormat::I16},
		{"sine440_flac24.flac", SampleFormat::I24},
	};

	for (const auto &[fileName, expectedFormat] : cases) {
		GIVEN("the path to " + fileName) {
			auto loaded = AudioFile::load(basicAudioPath(fileName).string());

			THEN("the SampleData format reflects the FLAC header depth") {
				REQUIRE(static_cast<bool>(loaded));
				REQUIRE(loaded.data.getFormat() == expectedFormat);
			}
		}
	}
}

SCENARIO_METHOD(AudioFileLoadFixture,
				"On Apple, AudioFile::load picks I16 for lossy MP3 and AAC",
				"[audio-file-load][apple]") {
	for (const auto &fileName : std::vector<std::string> {"sine440.mp3", "sine440_aac.m4a"}) {
		GIVEN("the path to " + fileName) {
			auto loaded = AudioFile::load(basicAudioPath(fileName).string());

			THEN("the SampleData format is I16") {
				REQUIRE(static_cast<bool>(loaded));
				REQUIRE(loaded.data.getFormat() == SampleFormat::I16);
			}
		}
	}
}

SCENARIO_METHOD(AudioFileLoadFixture,
				"On Apple, AudioFile::load retains ALAC native depth from the source header",
				"[audio-file-load][apple]") {
	GIVEN("the path to sine440_alac.m4a (16-bit ALAC)") {
		auto loaded = AudioFile::load(basicAudioPath("sine440_alac.m4a").string());

		THEN("the SampleData format is I16") {
			REQUIRE(static_cast<bool>(loaded));
			REQUIRE(loaded.data.getFormat() == SampleFormat::I16);
		}
	}
}

SCENARIO_METHOD(AudioFileLoadFixture,
				"On Apple, AudioFile::loadResampled retains the source PCM bit depth",
				"[audio-file-load][apple]") {
	GIVEN("a 16-bit WAV resampled to 22050 Hz") {
		auto loaded = AudioFile::loadResampled(basicAudioPath("sine440_pcm16.wav").string(), 22050);

		THEN("the SampleData format remains I16") {
			REQUIRE(static_cast<bool>(loaded));
			REQUIRE(loaded.data.getFormat() == SampleFormat::I16);
		}
	}

	GIVEN("a 24-bit WAV resampled to 22050 Hz") {
		auto loaded = AudioFile::loadResampled(basicAudioPath("sine440_pcm24.wav").string(), 22050);

		THEN("the SampleData format remains I24") {
			REQUIRE(static_cast<bool>(loaded));
			REQUIRE(loaded.data.getFormat() == SampleFormat::I24);
		}
	}
}

#endif
