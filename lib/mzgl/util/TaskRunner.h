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
    
    /**
     * For testing you can set the task runner to be synchronous - which will
     * just run functions passed to it rather than spinning up another thread
     * and dsipatching more threads off that.
     */
    TaskRunner(std::string name = "TaskRunner", bool synchronous = false) :
    name(name), synchronous(synchronous) {
        if(synchronous) return;
        
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
        if(synchronous) {
            taskFn();
            return;
        }
        if(!fut.valid()) {
            Log::e() << "TaskRunner '"<<name<<"' is finished!";
            return;
        }
        taskQueue.enqueue(taskFn);
        
	}

	// need to guarantee that ALL tasks are done before this dies.
	~TaskRunner() {
        if(synchronous) return;
        shouldStop.store(true);
        fut.wait();
		waitTilAllTasksAreDone();
	}

private:
    
    class Task {
    public:

        std::future<void> taskFuture;
        std::atomic<bool> isDone { false };
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
                isDone.store(true);
                // printf("finish task\n");
            });
            
        }
        
        bool done() {
//            return !taskFuture.valid();
            return isDone.load();
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
    
    std::future<void> fut;
    std::atomic<bool> shouldStop { false };
    std::thread thread;
    std::string name;
    bool synchronous = false;
};