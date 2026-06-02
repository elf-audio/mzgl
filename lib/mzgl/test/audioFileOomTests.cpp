#include "tests.h"

#include "AudioFile.h"
#include "RealtimeAllocChecker.h"
#include "filesystem.h"

#include <new>

// These tests are tagged [.oom] (hidden by default) rather than [oom] because
// Apple's CoreAudio internals are not exception-safe — when `operator new`
// throws std::bad_alloc inside ExtAudioFileOpenURL / ExtAudioFileRead, the
// exception passes through mixed Obj-C/C/C++ frames that don't expect it,
// leaving CoreAudio's internal state inconsistent. That corruption surfaces
// later as a crash in xzm's malloc freelist during an unrelated test (often
// AutoChop, whichever test next does a substantial allocation).
//
// The tests themselves still work in isolation:  `./tests "[.oom]"`
// — they pin the contract that the loader's noexcept wrappers surface
// LoadFailureFlag::OutOfMemory rather than letting bad_alloc escape. Just
// don't include them in the default full-sweep run.
#ifdef ENABLE_MALLOC_CHECKS

namespace {
	fs::path workingPath() {
		return fs::current_path() / "test-files" / "basic-audio" / "sine440_pcm16.wav";
	}
} // namespace

SCENARIO("AudioFile::load fails gracefully when allocations would fail",
		 "[audio-file-fail][.oom]") {
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
		 "[audio-file-fail][.oom]") {
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
