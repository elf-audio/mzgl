#pragma once

#include <mutex>
#include <shared_mutex>
#include <thread>
#include <atomic>
#include <optional>
#include <chrono>

enum class StackTraceMode { Error, Info };

#ifdef __clang__
#	include <execinfo.h>
inline void printInfoStack(char **symbols, int frames) {
	Log::i() << "Call stack:";
	for (int i = 4; i < frames; i++) {
		Log::i() << "  " << symbols[i];
	}
}

inline void printErrorStack(char **symbols, int frames) {
	Log::e() << "Call stack:";
	for (int i = 4; i < frames; i++) {
		Log::e() << "  " << symbols[i];
	}
}

inline void printStack(StackTraceMode mode) {
	static constexpr auto numFrames = 20;
	void *callstack[numFrames];
	int frames	   = backtrace(callstack, numFrames);
	char **symbols = backtrace_symbols(callstack, frames);

	if (mode == StackTraceMode::Error) {
		printErrorStack(symbols, frames);
	} else {
		printInfoStack(symbols, frames);
	}

	free(symbols);
}
#else
inline void printStack(StackTraceMode) {
}
#endif

#include "log.h"
#include "mzAssert.h"

#ifdef _MSC_VER
#	define FUNCTION_NAME __FUNCSIG__
#elif defined(__GNUC__) || defined(__clang__)
#	define FUNCTION_NAME __PRETTY_FUNCTION__
#else
#	define FUNCTION_NAME __func__
#endif

class ContentionCheckingMutex {
public:
	ContentionCheckingMutex() = default;

	ContentionCheckingMutex(const ContentionCheckingMutex &)			= delete;
	ContentionCheckingMutex &operator=(const ContentionCheckingMutex &) = delete;
	ContentionCheckingMutex(ContentionCheckingMutex &&)					= delete;
	ContentionCheckingMutex &operator=(ContentionCheckingMutex &&)		= delete;

	[[maybe_unused]] bool tryLockWrite(const std::string &description,
									   std::optional<int> triesBeforeFailure = std::nullopt) {
		using namespace std::chrono_literals;

		auto currentThread = std::this_thread::get_id();

		if (writerThread.load(std::memory_order_relaxed) == currentThread) {
			++writeLockCount;
			Log::i() << "-- CCM -- INFO: Recursive write lock from same thread";
			return true;
		}

		int tries = 0;
		while (!mutex.try_lock()) {
			if (tries == 0) {
				Log::e() << "-- CCM -- ERROR: Write contention detected!";
				Log::e() << "-- CCM -- ERROR: This thread: " << currentThread;
				Log::e() << "-- CCM -- ERROR: Writer thread: " << writerThread;
				Log::e() << description;
				printStack(StackTraceMode::Error);
				mzAssert(false);
			}

			if (triesBeforeFailure.has_value() && tries >= *triesBeforeFailure) {
				return false;
			}

			++tries;
			std::this_thread::sleep_for(100ms);
		}

		writerThread.store(currentThread, std::memory_order_relaxed);
		writeLockCount.store(1, std::memory_order_relaxed);

		Log::i() << "-- CCM -- INFO: Write lock acquired";
		Log::i() << description;
		printStack(StackTraceMode::Info);
		return true;
	}

	[[maybe_unused]] bool tryLockRead(const std::string &description,
									  std::optional<int> triesBeforeFailure = std::nullopt) {
		using namespace std::chrono_literals;

		int tries = 0;
		while (!mutex.try_lock_shared()) {
			if (tries == 0) {
				Log::e() << "-- CCM -- ERROR: Read contention detected! ";
				Log::e() << "-- CCM -- ERROR: This thread: " << std::this_thread::get_id();
				Log::e() << "-- CCM -- ERROR: Writer thread: " << writerThread;
				Log::e() << description;
				printStack(StackTraceMode::Error);
				mzAssert(false);
			}

			if (triesBeforeFailure.has_value() && tries >= *triesBeforeFailure) {
				return false;
			}

			++tries;
			std::this_thread::sleep_for(100ms);
		}

		activeReaders.fetch_add(1, std::memory_order_relaxed);

		Log::i() << "-- CCM -- INFO: Read lock acquired";
		Log::i() << description;
		printStack(StackTraceMode::Info);
		return true;
	}

	void unlockWrite() {
		auto currentThread = std::this_thread::get_id();

		if (writerThread.load(std::memory_order_relaxed) != currentThread) {
			Log::e() << "ERROR: Trying to unlock write mutex from wrong thread!";
			printStack(StackTraceMode::Error);
			mzAssert(false);
			return;
		}

		int newCount = --writeLockCount;
		if (newCount < 0) {
			Log::e() << "ERROR: writeLockCount went negative!";
			printStack(StackTraceMode::Error);
			mzAssert(false);
		}

		if (newCount == 0) {
			writerThread.store(std::thread::id(), std::memory_order_relaxed);
			mutex.unlock();
		}
	}

	void unlockRead() {
		auto newCount = activeReaders.fetch_sub(1, std::memory_order_relaxed) - 1;
		if (newCount < 0) {
			Log::e() << "ERROR: activeReaders went negative!";
			printStack(StackTraceMode::Error);
			mzAssert(false);
		}
		mutex.unlock_shared();
	}

private:
	std::shared_mutex mutex;

	std::atomic<std::thread::id> writerThread {};
	std::atomic<int> writeLockCount {0};

	std::atomic<int> activeReaders {0};
};

class ContentionCheckingWriteLockGuard {
public:
	ContentionCheckingWriteLockGuard(ContentionCheckingMutex &m,
									 const std::string &description,
									 std::optional<int> triesBeforeFailure = std::nullopt)
		: mutex {m} {
		locked = mutex.tryLockWrite(description, triesBeforeFailure);
	}

	~ContentionCheckingWriteLockGuard() {
		if (locked) {
			mutex.unlockWrite();
		}
	}

	ContentionCheckingWriteLockGuard(const ContentionCheckingWriteLockGuard &)			  = delete;
	ContentionCheckingWriteLockGuard &operator=(const ContentionCheckingWriteLockGuard &) = delete;

private:
	ContentionCheckingMutex &mutex;
	bool locked {false};
};

class ContentionCheckingReadLockGuard {
public:
	ContentionCheckingReadLockGuard(ContentionCheckingMutex &m,
									const std::string &description,
									std::optional<int> triesBeforeFailure = std::nullopt)
		: mutex {m} {
		locked = mutex.tryLockRead(description, triesBeforeFailure);
	}

	~ContentionCheckingReadLockGuard() {
		if (locked) {
			mutex.unlockRead();
		}
	}

	ContentionCheckingReadLockGuard(const ContentionCheckingReadLockGuard &)			= delete;
	ContentionCheckingReadLockGuard &operator=(const ContentionCheckingReadLockGuard &) = delete;

private:
	ContentionCheckingMutex &mutex;
	bool locked {false};
};