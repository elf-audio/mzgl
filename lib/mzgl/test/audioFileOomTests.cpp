#include "tests.h"

#include "AudioFile.h"
#include "RealtimeAllocChecker.h"
#include "filesystem.h"

#include <new>

#ifdef ENABLE_MALLOC_CHECKS

namespace {
	fs::path workingPath() {
		return fs::current_path() / "test-files" / "basic-audio" / "sine440_pcm16.wav";
	}
} // namespace

SCENARIO("AudioFile::load fails gracefully when allocations would fail",
		 "[audio-file-fail][oom]") {
	const auto path				 = workingPath();
	REQUIRE(fs::exists(path));
	const std::string pathString = path.string();

	AudioFile::LoadedAudio loaded;
	bool exceptionPropagated = false;

	WHEN("allocations inside load() are forced to fail") {
		auto runWithAllocFailure = [&]() {
			RealtimeAllocChecker::ScopedAllocationFailure scope;
			return AudioFile::load(pathString);
		};

		try {
			loaded = runWithAllocFailure();
		} catch (const std::bad_alloc &) {
			exceptionPropagated = true;
		} catch (const std::exception &) {
			exceptionPropagated = true;
		}

		THEN("no exception escapes the loader") {
			REQUIRE_FALSE(exceptionPropagated);
		}
		THEN("the load reports failure") {
			REQUIRE_FALSE(static_cast<bool>(loaded));
		}
		THEN("the LoadedAudio's failureFlags has the OutOfMemory bit set") {
			REQUIRE((loaded.failureFlags & AudioFile::LoadFailureFlag::OutOfMemory) != 0);
		}
		THEN("no audio data is returned") {
			REQUIRE(loaded.data.empty());
		}
	}
}

SCENARIO("AudioFile::loadResampled fails gracefully when allocations would fail",
		 "[audio-file-fail][oom]") {
	const auto path				 = workingPath();
	REQUIRE(fs::exists(path));
	const std::string pathString = path.string();

	AudioFile::LoadedAudio loaded;
	bool exceptionPropagated = false;

	WHEN("allocations inside loadResampled() are forced to fail") {
		auto runWithAllocFailure = [&]() {
			RealtimeAllocChecker::ScopedAllocationFailure scope;
			return AudioFile::loadResampled(pathString, 48000);
		};

		try {
			loaded = runWithAllocFailure();
		} catch (const std::bad_alloc &) {
			exceptionPropagated = true;
		} catch (const std::exception &) {
			exceptionPropagated = true;
		}

		THEN("no exception escapes the loader") {
			REQUIRE_FALSE(exceptionPropagated);
		}
		THEN("the load reports failure") {
			REQUIRE_FALSE(static_cast<bool>(loaded));
		}
		THEN("the LoadedAudio's failureFlags has the OutOfMemory bit set") {
			REQUIRE((loaded.failureFlags & AudioFile::LoadFailureFlag::OutOfMemory) != 0);
		}
		THEN("no audio data is returned") {
			REQUIRE(loaded.data.empty());
		}
	}
}

#endif
