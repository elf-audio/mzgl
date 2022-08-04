//
//  mainThread.cpp
//  mzgl
//
//  Created by Marek Bereza on 04/08/2022.
//  Copyright © 2022 Marek Bereza. All rights reserved.
//

#include "mainThread.h"

#include "concurrentqueue.h"
#include <thread>
#include "log.h"
#include "mzAssert.h"
static std::thread::id		mainThreadId;
using namespace std;

// from cinder
bool isMainThread() {
	return std::this_thread::get_id() == mainThreadId;
}

void setMainThreadId() {
	mainThreadId = std::this_thread::get_id();
}

moodycamel::ConcurrentQueue<function<void()>> mainThreadQueue;

void runOnMainThread(function<void()> fn) {
	mzAssert(!isMainThread());
	mainThreadQueue.enqueue(fn);
}


void runOnMainThreadAndWait(function<void()> fn) {
	if(isMainThread()) {
		Log::e() << "runOnMainThreadAndWait() called from main thread";
		fn();
		return;
	}
	mzAssert(!isMainThread());
	atomic<bool> done {false};
	mainThreadQueue.enqueue([fn,&done]() {
		fn();
		done.store(true);
	});
	
	while(!done.load()) {
		std::this_thread::sleep_for(std::chrono::microseconds(100));
	}
}


void runOnMainThread(bool checkIfOnMainThread, function<void()> fn) {
	if(checkIfOnMainThread && isMainThread()) {
		fn();
	} else {
		runOnMainThread(fn);
	}
}
#ifdef UNIT_TEST
// dangerous to use in anything but testing
void clearMainThreadQueue() {
	mzAssert(isMainThread());
	function<void()> fn;
	while(mainThreadQueue.try_dequeue(fn)) {}
}
#endif

void pollMainThreadQueue() {
	function<void()> fn;
	while(mainThreadQueue.try_dequeue(fn)) {
		fn();
	}
}
