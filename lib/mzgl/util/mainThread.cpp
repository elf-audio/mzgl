//
//  mainThread.cpp
//  mzgl
//
//  Created by Marek Bereza on 04/08/2022.
//  Copyright © 2022 Marek Bereza. All rights reserved.
//

#include "mainThread.h"
#include "log.h"
#include "mzAssert.h"
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
void MainThreadRunner::runOnNextMainLoop(std::function<void()> fn) {
	mzAssert(isMainThread());
	mainThreadQueue->enqueue(fn);
}
#include "util.h"
void MainThreadRunner::runOnMainThreadAndWait(std::function<void()> fn, bool logToLockfile) {
	if (logToLockfile) writeToLockFile("101.1");
	if (isMainThread()) {
		if (logToLockfile) writeToLockFile("101.2");
		Log::e() << "runOnMainThreadAndWait() called from main thread";
		fn();
		if (logToLockfile) writeToLockFile("101.3");
		return;
	}
	if (logToLockfile) writeToLockFile("101.4");
	mzAssert(!isMainThread());
	if (logToLockfile) writeToLockFile("101.5");
	std::atomic<bool> done {false};

	mainThreadQueue->enqueue([fn, &done, logToLockfile]() {
		if (logToLockfile) writeToLockFile("101.5");
		fn();
		if (logToLockfile) writeToLockFile("101.6");
		done.store(true);
		if (logToLockfile) writeToLockFile("101.7");
	});
	if (logToLockfile) writeToLockFile("101.8");
	int originalPollCount = pollCount;

	const uint64_t sleepTime = 100;
	uint64_t duration		 = 0;

	while (!done.load()) {
		std::this_thread::sleep_for(std::chrono::microseconds(sleepTime));
		duration += sleepTime;
		if (logToLockfile) writeToLockFile("101.9");
		// this may never have polled
		if (duration > 1'000'000 && pollCount == 0) {
			Log::e() << "Main thread may be not active";
			pollMainThreadQueue();
			if (logToLockfile) writeToLockFile("101.10");
		}
	}
	if (logToLockfile) writeToLockFile("101.11");
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
