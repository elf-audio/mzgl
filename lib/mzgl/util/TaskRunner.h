//
//  TaskRunner.h
//  koala
//
//  Created by Marek Bereza on 22/07/2022.
//  Copyright © 2022 Marek Bereza. All rights reserved.
//

#pragma once

#include <list>
#include <future>
#include <thread>
#include "log.h"
#include "mainThread.h"
#include "concurrentqueue.h"

/**
 * runTask used to be a global function with static members.
 * This is a problem for environments where there is more than
 * one instance of koala running (e.g. as a plugin), so
 * now it lives within a class that has a defined lifetime.
 */
class TaskRunner {
public:
    
private:
//	class Task {
//	public:
//
//		std::future<void> taskFuture;
//		std::atomic<bool> done { false };
//
//		Task(TaskRunner *runner, std::function<void()> taskFn) {
//
//			taskFuture = std::async(std::launch::async, [this, taskFn, runner](){
// #if defined(__APPLE__) && DEBUG
//				// sets a thread name that is readable in xcode when debugging
//				pthread_setname_np("runTask()");
// #endif
//				try {
//					taskFn();
//				} catch(const std::exception& err) {
//					std::string ex = err.what();
////					runner->main.runOnMainThread([ex]() {
//						Log::e() << "exception in runTask: " << ex;
////					});
//
//				}
//
//				done.store(true);
//
//				// this could be a problem if we're the last task and things are getting destructed
////				runner->main.runOnMainThread([runner]() {
//					runner->clearAnyDoneTasks();
////				});
//			});
//		}
//	};
    
public:
    
//    std::future<void> fut;
    std::atomic<bool> shouldStop { false };
    std::thread thread;
    TaskRunner() {
        
        thread = std::thread([this](){
#ifdef __APPLE__
            pthread_setname_np("TaskRunner");
#endif
            std::function<void()> taskFn;
            
            while(!shouldStop) {
                while(taskQueue.try_dequeue(taskFn)) {
        //            tasks.emplace_back(std::make_shared<Task>(taskFn));
                    taskFn();
                }
                
                std::this_thread::sleep_for(std::chrono::microseconds(100));
            }
        });
    }
	// runs taskFn asynchronously, run on any thread
	void run(std::function<void()> taskFn) {
        taskQueue.enqueue(taskFn);
        
//		auto addTaskFn = [taskFn, this]() {
//			clearAnyDoneTasks();
//			tasks.emplace_back(std::make_shared<Task>(this, taskFn));
//		};

		// this will run straight away if called on main
		// thread or defer it to the mainThreadQueue if not.
//		main.runOnMainThread(true, addTaskFn);
//        addTaskFn();
	}

	// need to guarantee that ALL tasks are done before this dies.
	~TaskRunner() {
        shouldStop = true;
        thread.join();
//		waitTilAllTasksAreDone();
	}
    
    void update() {
        std::function<void()> taskFn;
        while(taskQueue.try_dequeue(taskFn)) {
//            tasks.emplace_back(std::make_shared<Task>(taskFn));
            taskFn();
        }
    }

	void waitTilAllTasksAreDone() {
//		while(tasks.size()>0) {
//			clearAnyDoneTasks();
//			std::this_thread::sleep_for(std::chrono::microseconds(100));
//		}
	}

	void clearAnyDoneTasks() {
//		tasks.remove_if([](const std::shared_ptr<Task> task) { return task->done.load();});
	}

private:
    moodycamel::ConcurrentQueue<std::function<void()>> taskQueue;
//	std::list<std::shared_ptr<Task>> tasks;
};
