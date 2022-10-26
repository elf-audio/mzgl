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
    
    std::future<void> fut;
    std::atomic<bool> shouldStop { false };
    std::thread thread;
    std::string name;
    
    TaskRunner(std::string name = "TaskRunner") : name(name) {
        
        fut = std::async(std::launch::async, [this](){
#ifdef __APPLE__
            pthread_setname_np(this->name.c_str());
#endif
            std::function<void()> taskFn;
            
            int ppp = 0;
            while(!shouldStop) {
                
                if(ppp++>10) {
                    ppp = 0;
                    clearAnyDoneTasks();
                }
                
                while(taskQueue.try_dequeue(taskFn)) {
                    tasks.emplace_back(std::make_shared<Task>(taskFn));
                }
                
                std::this_thread::sleep_for(std::chrono::microseconds(100));
            }
        });
    }
    
	// runs taskFn asynchronously, run on any thread
	void run(std::function<void()> taskFn) {
        if(!fut.valid()) {
            Log::e() << "TaskRunner '"<<name<<"' is finished!";
            return;
        }
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
        shouldStop.store(true);
        fut.wait();
		waitTilAllTasksAreDone();
	}
    
//    void update() {
//        std::function<void()> taskFn;
//        while(taskQueue.try_dequeue(taskFn)) {
////            tasks.emplace_back(std::make_shared<Task>(taskFn));
//            taskFn();
//        }
//    }
//
	

private:
    
    
    class Task {
    public:

        std::future<void> taskFuture;
        
//#if UNIT_TEST
//        std::function<void()> tf;
//#endif
        Task(std::function<void()> taskFn) {
//#if UNIT_TEST
//            tf = taskFn;
//#endif
            taskFuture = std::async(std::launch::async, [this, taskFn](){
 #if defined(__APPLE__) && (DEBUG || UNIT_TEST)
                // sets a thread name that is readable in xcode when debugging
                pthread_setname_np("runTask()");
 #endif
                try {
                    taskFn();
                } catch(const std::exception& err) {
                    std::string ex = err.what();
                    Log::e() << "exception in runTask: " << ex;
                }
                
                // printf("finish task\n");
            });
            
        }
        
        bool done() {
            return !taskFuture.valid();
        }
    };
    
    
   

    void clearAnyDoneTasks() {
        tasks.remove_if([](const std::shared_ptr<Task> task) {
            return task->done();
        });
    }
    
    moodycamel::ConcurrentQueue<std::function<void()>> taskQueue;
	std::list<std::shared_ptr<Task>> tasks;

#ifdef UNIT_TEST
public:
    void clearTasks() {
        // really dangerous, only for unit testing
        tasks.clear();
    }
#endif
    void waitTilAllTasksAreDone() {
        while(tasks.size()>0) {
            clearAnyDoneTasks();
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
    }
};
