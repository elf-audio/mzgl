//
//  mainThread.cpp
//  mzgl
//
//  Created by Marek Bereza on 04/08/2022.
//  Copyright © 2022 Marek Bereza. All rights reserved.
//

#include <mzgl/util/mainThread.h>
#include <mzgl/util/log.h>
#include <mzgl/util/mzAssert.h>
#include "concurrentqueue.h"

class MainThreadRunner::LambdaQueue : public moodycamel::ConcurrentQueue<std::function<void()>> {};
// from cinder
MainThreadRunner::MainThreadRunner() {
	mainThreadQueue = std::make_shared<LambdaQueue>();
}
MainThreadRunner::~MainThreadRunner() {
	//mzAssert(isMainThread());
	clearMainThreadQueue();
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

void MainThreadRunner::runOnMainThread(std::function<void()> fn) {
	//	Log::d() << mainThreadId << " " << std::this_thread::get_id();
	mzAssert(!isMainThread());
	mainThreadQueue->enqueue(fn);
}

void MainThreadRunner::runOnMainThreadAndWait(std::function<void()> fn) {
	if (isMainThread()) {
		Log::e() << "runOnMainThreadAndWait() called from main thread";
		fn();
		return;
	}
	mzAssert(!isMainThread());
	std::atomic<bool> done {false};
	mainThreadQueue->enqueue([fn, &done]() {
		fn();
		done.store(true);
	});
	int originalPollCount = pollCount;

	const uint64_t sleepTime = 100;
	uint64_t duration		 = 0;
	while (!done.load()) {
		std::this_thread::sleep_for(std::chrono::microseconds(sleepTime));
		duration += sleepTime;

		// this may never have polled
		if (duration > 1'000'000 && pollCount == 0) {
			Log::e() << "Main thread may be not active";
			pollMainThreadQueue();
		}
	}
}

void MainThreadRunner::runOnMainThread(bool checkIfOnMainThread, std::function<void()> fn) {
	if (checkIfOnMainThread && isMainThread()) {
		fn();
	} else {
		runOnMainThread(fn);
	}
}

// dangerous to use in anything but testing
void MainThreadRunner::clearMainThreadQueue() {
	//mzAssert(isMainThread());
	std::function<void()> fn;
	while (mainThreadQueue->try_dequeue(fn)) {}
}

void MainThreadRunner::pollInternal() {
	std::function<void()> fn;
	pollMutex.lock();
	while (mainThreadQueue->try_dequeue(fn)) {
		fn();
	}
	pollMutex.unlock();
}

void MainThreadRunner::pollMainThreadQueue() {
	if (!hasSetMainThreadId) {
		hasSetMainThreadId = true;
		setMainThreadId();
	}

	pollCount++;
	pollInternal();
}
