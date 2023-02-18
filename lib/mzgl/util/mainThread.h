//
//  mainThread.h
//  mzgl
//
//  Created by Marek Bereza on 04/08/2022.
//  Copyright © 2022 Marek Bereza. All rights reserved.
//

#pragma once

#include <functional>
#include <thread>
#include <mutex>
#include <memory>

class MainThreadRunner {
public:
	MainThreadRunner();
	virtual ~MainThreadRunner();
	
	bool isMainThread();
	
	void runOnMainThread(std::function<void()> fn);
	
	/**
	 * if the call to this is made on the main thread,
	 * this just runs it rather than enqueueing it
	 * on the mainThreadQueue
	 */
	void runOnMainThread(bool checkIfOnMainThread, std::function<void()> fn);
	
	/**
	 * This will block your current thread until the fn()
	 * has run on the main thread.
	 */
	void runOnMainThreadAndWait(std::function<void()> fn);
	
	/**
	 * only use this if you want to change which
	 * thread is marked as the "main" thread -
	 * all you have to do is call it on the
	 * thread you want to set.
	 */
	void setMainThreadId();
	
	/**
	 * this is called internally on the
	 *  main thread to run all the things
	 *  that have been added with runOnMainThread
	 */
	void pollMainThreadQueue();
	
#ifndef UNIT_TEST
private:
#endif
	// dangerous to use in anything but testing
	void clearMainThreadQueue();
	
private:
	// helps for complex fwd declaration
	class LambdaQueue;
	std::thread::id		mainThreadId;
	std::shared_ptr<LambdaQueue> mainThreadQueue;
//	bool mainThreadEverPolled = false;
	bool hasSetMainThreadId = false;
	std::atomic<int> pollCount {0};
	std::mutex pollMutex;
	void pollInternal();
};
