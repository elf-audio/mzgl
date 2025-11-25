#include "mainThread.h"
#include "log.h"
#include "mzAssert.h"
#include "concurrentqueue.h"
#include "util.h"
#include "mzgl_platform.h"

class MainThreadRunner::LambdaQueue : public moodycamel::ConcurrentQueue<std::function<void()>> {};

MainThreadRunner::MainThreadRunner() {
	mainThreadQueue = std::make_shared<LambdaQueue>();
}

MainThreadRunner::~MainThreadRunner() {
	requestStop(std::chrono::milliseconds {500});
}

bool MainThreadRunner::isMainThread() {
	return std::this_thread::get_id() == mainThreadId;
}

void MainThreadRunner::setMainThreadId() {
	setMainThreadId(std::this_thread::get_id());
}

void MainThreadRunner::setMainThreadId(std::thread::id threadId) {
	mainThreadId = threadId;
}

bool MainThreadRunner::runOnMainThread(std::function<void()> fn) {
	mzAssert(!isMainThread());

	auto s = state.load(std::memory_order_acquire);
	if (s != State::Running) {
		return false;
	}

	pending.fetch_add(1, std::memory_order_acq_rel);
	if (!mainThreadQueue->enqueue(std::move(fn))) {
		pending.fetch_sub(1, std::memory_order_acq_rel);
		std::lock_guard<std::mutex> lk(waitMx);
		waitCv.notify_all();
		return false;
	}
	return true;
}

bool MainThreadRunner::runOnNextMainLoop(std::function<void()> fn) {
	auto s = state.load(std::memory_order_acquire);
	if (s != State::Running) {
		return false;
	}

	pending.fetch_add(1, std::memory_order_acq_rel);
	if (!mainThreadQueue->enqueue(std::move(fn))) {
		pending.fetch_sub(1, std::memory_order_acq_rel);
		std::lock_guard<std::mutex> lk(waitMx);
		waitCv.notify_all();
		return false;
	}

	return true;
}

bool MainThreadRunner::runIfMainThread(std::function<void()> fn,
									   const std::string &description,
									   bool logToLockfile,
									   const std::string &prepend) {
	if (!isMainThread()) {
		return false;
	}

	Log::w() << "runOnMainThreadAndWait() called -> " << description;

	if (logToLockfile) {
		writeToLockFile(prepend + "2");
	}

	fn();

	if (logToLockfile) {
		writeToLockFile(prepend + "3");
	}
	return true;
}

bool MainThreadRunner::runOnMainThreadAndWait(std::function<void()> fn,
											  bool logToLockfile,
											  const std::string &prepend) {
	if (logToLockfile) {
		writeToLockFile(prepend + "1");
	}

	if (state.load(std::memory_order_acquire) != State::Running) {
		if (runIfMainThread(fn, "when not running", logToLockfile, prepend)) {
			return true;
		}
		return false;
	}

	if (runIfMainThread(fn, "from main thread", logToLockfile, prepend)) {
		return true;
	}

	if (logToLockfile) {
		writeToLockFile(prepend + "4");
	}

	if (logToLockfile) {
		writeToLockFile(prepend + "5");
	}

	std::atomic<bool> done {false};
	if (!runOnMainThread([&]() {
			if (logToLockfile) {
				writeToLockFile(prepend + "5");
			}

			fn();

			if (logToLockfile) {
				writeToLockFile(prepend + "6");
			}

			done.store(true, std::memory_order_release);

			if (logToLockfile) {
				writeToLockFile(prepend + "7");
			}
		})) {
		return false;
	}

	if (logToLockfile) {
		writeToLockFile(prepend + "8");
	}

	if (isMainThread()) {
		while (!done.load(std::memory_order_acquire)) {
			pollInternal();
		}
	} else {
		while (!done.load(std::memory_order_acquire)) {
			std::this_thread::sleep_for(std::chrono::microseconds(200));
		}
	}

	if (logToLockfile) {
		writeToLockFile(prepend + "11");
	}
	return true;
}

bool MainThreadRunner::runOnMainThread(bool checkIfOnMainThread, std::function<void()> fn) {
	if (checkIfOnMainThread && isMainThread()) {
		fn();
		return true;
	}
	return runOnMainThread(fn);
}

bool MainThreadRunner::isIdle(const std::atomic<uint32_t> &pending, const std::atomic<bool> &inCb) {
	return pending.load(std::memory_order_acquire) == 0 && !inCb.load(std::memory_order_acquire);
}

void MainThreadRunner::requestStop(std::optional<std::chrono::milliseconds> timeout) {
	State expected = State::Running;
	if (state.compare_exchange_strong(expected, State::Stopping, std::memory_order_acq_rel)) {
		Log::d() << "Main thread runner marked as stopping";
	}

	if (!timeout.has_value()) {
		Log::d() << "Main thread runner waiting indefinitely for jobs to finish";
		if (isMainThread()) {
			while (!isIdle(pending, inCallback)) {
				pollInternal();
			}
			state.store(State::Stopped, std::memory_order_release);
			return;
		}

		std::unique_lock<std::mutex> lk(waitMx);
		waitCv.wait(lk, [&]() { return isIdle(pending, inCallback); });
		state.store(State::Stopped, std::memory_order_release);
		Log::d() << "Main thread runner is finished";
		return;
	}

	const auto deadline = std::chrono::steady_clock::now()
						  + std::chrono::duration_cast<std::chrono::steady_clock::duration>(*timeout);

	if (isMainThread()) {
		while (!isIdle(pending, inCallback)) {
			if (std::chrono::steady_clock::now() >= deadline) {
				Log::e() << "Main thread runner ran out of time to execute shutdown jobs";
				break;
			}
			pollInternal();
		}

		if (!isIdle(pending, inCallback)) {
			executeWhileStopping.store(false, std::memory_order_release);
			Log::e() << "Main thread runner ran out of time so waiting a very short period for current job";
			while (inCallback.load(std::memory_order_acquire)
				   && std::chrono::steady_clock::now() < deadline + std::chrono::milliseconds(250)) {}

			clearMainThreadQueue();
			state.store(State::Stopped, std::memory_order_release);
			Log::e() << "Main thread runner was force stopped";
			return;
		}
		state.store(State::Stopped, std::memory_order_release);
		Log::d() << "Main thread runner has finished";
		return;
	}
	Log::d() << "Stopping main thread. We are not on the real \"main\"";

	std::unique_lock<std::mutex> lk(waitMx);
	bool graceful = waitCv.wait_until(lk, deadline, [&] { return isIdle(pending, inCallback); });
	lk.unlock();

	if (graceful) {
		state.store(State::Stopped, std::memory_order_release);
		Log::d() << "Main thread runner was stopped";
		return;
	}

	executeWhileStopping.store(false, std::memory_order_release);
	clearMainThreadQueue();
	auto end = deadline + std::chrono::milliseconds(250);
	while (inCallback.load(std::memory_order_acquire) && std::chrono::steady_clock::now() < end) {
		std::this_thread::sleep_for(std::chrono::milliseconds(5));
	}
	state.store(State::Stopped, std::memory_order_release);
	Log::e() << "Main thread runner was force stopped";
}

void MainThreadRunner::clearMainThreadQueue() {
	std::function<void()> fn;
	uint32_t dropped = 0;
	while (mainThreadQueue->try_dequeue(fn)) {
		++dropped;
	}
	if (dropped) {
		pending.fetch_sub(dropped, std::memory_order_acq_rel);
		std::lock_guard<std::mutex> lk(waitMx);
		waitCv.notify_all();
	}
}

void MainThreadRunner::pollInternal() {
	if (!isMainThread()) {
		return;
	}

	std::function<void()> fn;
	size_t budget = 1024;

	while (budget--) {
		if (state.load(std::memory_order_acquire) != State::Running
			&& !executeWhileStopping.load(std::memory_order_acquire)) {
			break;
		}

		if (!mainThreadQueue->try_dequeue(fn)) {
			break;
		}

		inCallback.store(true, std::memory_order_release);
		try {
			fn();
		} catch (std::exception &e) {
			Log::e() << "Caught unhandled exception in main thread polling -> " << e.what();
		} catch (...) {
			Log::e() << "Caught unhandled exception in main thread polling";
		}

		inCallback.store(false, std::memory_order_release);
		pending.fetch_sub(1, std::memory_order_acq_rel);

		{
			std::lock_guard<std::mutex> lk(waitMx);
			waitCv.notify_all();
		}
	}
}

void MainThreadRunner::pollMainThreadQueue() {
	testAndSetMainThreadId();
	pollInternal();
}

void MainThreadRunner::testAndSetMainThreadId() {
#if MZGL_MAC
	/**
	 * Note: 24/11/2025
	 * In open GL we are guaranteed that the callbacks come from a single "main" thread
	 * However, when using Metal, the callbacks can come from any thread, and the OS can
	 * move this around. On Intel mac specifically, with an integrated dual graphics card
	 * the thread id will be different after the first call.
	 * Setting this every call to poll internal is required to make sure that all
	 * future tests work in the way we expect
	 * Other platforms appear unaffected
	 */
	setMainThreadId();
#else
	bool expected = false;
	if (hasSetMainThreadId.compare_exchange_strong(expected, true)) {
		setMainThreadId();
	}
#endif
	mzAssert(isMainThread(), "Must be called on the real main thread");
}
