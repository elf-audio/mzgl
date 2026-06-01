#include "tests.h"

#include "AudioFile.h"
#include "filesystem.h"

#include <string>
#include <vector>

#ifndef _WIN32
#	include <fcntl.h>
#	include <unistd.h>
#endif

namespace {
	class SilencedStderr {
	public:
		SilencedStderr() {
#ifndef _WIN32
			savedFd				= dup(STDERR_FILENO);
			const int devNullFd = open("/dev/null", O_WRONLY);
			dup2(devNullFd, STDERR_FILENO);
			close(devNullFd);
#endif
		}
		~SilencedStderr() {
#ifndef _WIN32
			dup2(savedFd, STDERR_FILENO);
			close(savedFd);
#endif
		}

	private:
		int savedFd = 0;
	};

	std::vector<fs::path> filesInDir(const fs::path &dir) {
		std::vector<fs::path> out;
		for (const auto &entry : fs::directory_iterator(dir)) {
			if (entry.is_regular_file()) out.push_back(entry.path());
		}
		return out;
	}

	fs::path badFilesDir() {
		return fs::current_path() / "test-files" / "bad-files";
	}

	fs::path invalidFilesDir() {
		return fs::current_path() / "test-files" / "invalid-files";
	}

	fs::path curiositiesDir() {
		return fs::current_path() / "test-files" / "curiosities";
	}
} // namespace

SCENARIO("AudioFile::load fails for every empty file in invalid-files", "[audio-file-fail]") {
	SilencedStderr scope;
	const auto files = filesInDir(invalidFilesDir());

	GIVEN("the directory of empty test files") {
		REQUIRE_FALSE(files.empty());

		WHEN("each file is loaded with AudioFile::load") {
			THEN("every load reports failure, populates at least one issue, and yields no data") {
				for (const auto &path : files) {
					AudioFile::LoadedAudio loaded;
					REQUIRE_NOTHROW(loaded = AudioFile::load(path.string()));
					CAPTURE(path.string());
					REQUIRE_FALSE(static_cast<bool>(loaded));
					REQUIRE_FALSE(loaded.result.success());
					REQUIRE_FALSE(loaded.result.issues.empty());
					REQUIRE(loaded.data.empty());
				}
			}
		}
	}
}

SCENARIO("AudioFile::loadResampled fails for every empty file in invalid-files",
		 "[audio-file-fail]") {
	SilencedStderr scope;
	const auto files = filesInDir(invalidFilesDir());

	GIVEN("the directory of empty test files") {
		REQUIRE_FALSE(files.empty());

		WHEN("each file is loaded with AudioFile::loadResampled at 48000 Hz") {
			THEN("every load reports failure, populates at least one issue, and yields no data") {
				for (const auto &path : files) {
					AudioFile::LoadedAudio loaded;
					REQUIRE_NOTHROW(loaded = AudioFile::loadResampled(path.string(), 48000));
					CAPTURE(path.string());
					REQUIRE_FALSE(static_cast<bool>(loaded));
					REQUIRE_FALSE(loaded.result.success());
					REQUIRE_FALSE(loaded.result.issues.empty());
					REQUIRE(loaded.data.empty());
				}
			}
		}
	}
}

SCENARIO("AudioFile::load fails for every corrupt file in bad-files", "[audio-file-fail]") {
	SilencedStderr scope;
	const auto files = filesInDir(badFilesDir());

	GIVEN("the directory of corrupt test files") {
		REQUIRE_FALSE(files.empty());

		WHEN("each file is loaded with AudioFile::load") {
			THEN("every load reports failure and populates at least one issue") {
				for (const auto &path : files) {
					AudioFile::LoadedAudio loaded;
					REQUIRE_NOTHROW(loaded = AudioFile::load(path.string()));
					CAPTURE(path.string());
					REQUIRE_FALSE(static_cast<bool>(loaded));
					REQUIRE_FALSE(loaded.result.success());
					REQUIRE_FALSE(loaded.result.issues.empty());
				}
			}
		}
	}
}

SCENARIO("AudioFile::loadResampled fails for every corrupt file in bad-files",
		 "[audio-file-fail]") {
	SilencedStderr scope;
	const auto files = filesInDir(badFilesDir());

	GIVEN("the directory of corrupt test files") {
		REQUIRE_FALSE(files.empty());

		WHEN("each file is loaded with AudioFile::loadResampled at 48000 Hz") {
			THEN("every load reports failure and populates at least one issue") {
				for (const auto &path : files) {
					AudioFile::LoadedAudio loaded;
					REQUIRE_NOTHROW(loaded = AudioFile::loadResampled(path.string(), 48000));
					CAPTURE(path.string());
					REQUIRE_FALSE(static_cast<bool>(loaded));
					REQUIRE_FALSE(loaded.result.success());
					REQUIRE_FALSE(loaded.result.issues.empty());
				}
			}
		}
	}
}

SCENARIO("AudioFile::load on a non-existent path returns a failure with an issue",
		 "[audio-file-fail]") {
	SilencedStderr scope;
	const auto path = fs::current_path() / "test-files" / "this-file-does-not-exist.wav";
	REQUIRE_FALSE(fs::exists(path));

	WHEN("AudioFile::load is called") {
		AudioFile::LoadedAudio loaded;
		REQUIRE_NOTHROW(loaded = AudioFile::load(path.string()));

		THEN("the load reports failure") {
			REQUIRE_FALSE(static_cast<bool>(loaded));
			REQUIRE_FALSE(loaded.result.success());
		}
		THEN("at least one issue is recorded") {
			REQUIRE_FALSE(loaded.result.issues.empty());
		}
		THEN("no data is returned") {
			REQUIRE(loaded.data.empty());
		}
	}
}

SCENARIO("AudioFile::loadResampled on a non-existent path returns a failure with an issue",
		 "[audio-file-fail]") {
	SilencedStderr scope;
	const auto path = fs::current_path() / "test-files" / "this-file-does-not-exist.wav";
	REQUIRE_FALSE(fs::exists(path));

	WHEN("AudioFile::loadResampled is called") {
		AudioFile::LoadedAudio loaded;
		REQUIRE_NOTHROW(loaded = AudioFile::loadResampled(path.string(), 48000));

		THEN("the load reports failure") {
			REQUIRE_FALSE(static_cast<bool>(loaded));
			REQUIRE_FALSE(loaded.result.success());
		}
		THEN("at least one issue is recorded") {
			REQUIRE_FALSE(loaded.result.issues.empty());
		}
		THEN("no data is returned") {
			REQUIRE(loaded.data.empty());
		}
	}
}

SCENARIO("curiosities/blank.wav: a header-only WAV loads successfully with no audio frames",
		 "[audio-file-fail]") {
	SilencedStderr scope;
	const auto path = curiositiesDir() / "blank.wav";
	REQUIRE(fs::exists(path));

	WHEN("AudioFile::load is called") {
		AudioFile::LoadedAudio loaded;
		REQUIRE_NOTHROW(loaded = AudioFile::load(path.string()));

		THEN("the load reports success (the file has a valid WAV header)") {
			REQUIRE(static_cast<bool>(loaded));
			REQUIRE(loaded.result.success());
		}
		THEN("the data buffer is empty (zero audio frames)") {
			REQUIRE(loaded.data.empty());
		}
	}

	WHEN("AudioFile::loadResampled is called") {
		AudioFile::LoadedAudio loaded;
		REQUIRE_NOTHROW(loaded = AudioFile::loadResampled(path.string(), 48000));

		THEN("the load reports success") {
			REQUIRE(static_cast<bool>(loaded));
			REQUIRE(loaded.result.success());
		}
		THEN("the data buffer is empty") {
			REQUIRE(loaded.data.empty());
		}
	}
}
