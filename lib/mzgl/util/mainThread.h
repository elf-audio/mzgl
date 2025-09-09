#pragma once

#include <functional>
#include <thread>
#include <mutex>
#include <memory>
#include <atomic>

class MainThreadRunner final {
public:
	MainThreadRunner();
	~MainThreadRunner();

	/**
	 * Run the fn on the main thread.
	 * This will place the function on the queue for running on the next main thread pump
	 * @param fn The function to run. Be very careful about capturing this in this function!
	 */
	[[maybe_unused]] bool runOnMainThread(std::function<void()> fn);

	/**
	 * if the call to this is made on the main thread,
	 * this just runs it rather than enqueueing it
	 * on the mainThreadQueue
	 */
	[[maybe_unused]] bool runOnMainThread(bool checkIfOnMainThread, std::function<void()> fn);

	/**
	 * This will block your current thread until the fn()
	 * has run on the main thread.
	 */
	[[maybe_unused]] bool runOnMainThreadAndWait(std::function<void()> fn,
												 bool logToLockfile			= false,
												 const std::string &prepend = "");

	/**
	 * Call only from main thread, it will run on the next loop round.
	 */
	[[maybe_unused]] bool runOnNextMainLoop(std::function<void()> fn);

	/**
	 * only use this if you want to change which
	 * thread is marked as the "main" thread -
	 * all you have to do is call it on the
	 * thread you want to set.
	 */
	void setMainThreadId();
	void setMainThreadId(std::thread::id threadId);
	[[nodiscard]] bool isMainThread();

	/**
	 * this is called internally on the
	 * main thread to run all the things
	 * that have been added with runOnMainThread
	 */
	void pollMainThreadQueue();

	/** clears any pending functions
	 * on the main thread queue
	 * (only call this from the main thread)
	 * @param timeout If std::nullopt, will wait for all jobs to finish.
	 *                If set to a value, will wait that time before it stops
	 */
	void requestStop(std::optional<std::chrono::milliseconds> timeout);

private:
	void testAndSetMainThreadId();
	void clearMainThreadQueue();
	void pollInternal();
	[[nodiscard]] bool runIfMainThread(std::function<void()> fn,
									   const std::string &description,
									   bool logToLockfile		  = false,
									   const std::string &prepend = "");
	static bool isIdle(const std::atomic<uint32_t> &pending, const std::atomic<bool> &inCb);

	class LambdaQueue;
	std::thread::id mainThreadId;
	std::shared_ptr<LambdaQueue> mainThreadQueue;
	std::atomic<bool> hasSetMainThreadId {false};
	std::mutex pollMutex;

	enum class State : uint8_t { Running, Stopping, Stopped };
	std::atomic<State> state {State::Running};
	std::atomic<uint32_t> pending {0};
	std::atomic<bool> inCallback {false};
	std::atomic<bool> executeWhileStopping {true};
	std::mutex waitMx;
	std::condition_variable waitCv;
};
