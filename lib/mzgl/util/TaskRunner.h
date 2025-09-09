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

using namespace std::chrono_literals;

/**
 * runTask used to be a global function with static members.
 * This is a problem for environments where there is more than
 * one instance of koala running (e.g. as a plugin), so
 * now it lives within a class that has a defined lifetime.
 */
class TaskRunner final {
public:
	using task_t	= std::function<void()>;
	using done_cb_t = std::function<void()>;

	struct TaskSpec {
		task_t task;
		done_cb_t onDone;
	};

	TaskRunner(const std::string &_name = "TaskRunner", bool _synchronous = false);
	~TaskRunner();

	void run(TaskSpec spec);
	void requestStop(std::chrono::milliseconds timeout = std::chrono::seconds(5));

private:
	class Task final {
	public:
		Task(TaskSpec spec);
		[[nodiscard]] bool done();

	private:
		std::future<void> taskFuture;
	};

	void createFuture();
	void clearAnyDoneTasks();
	void waitTilAllTasksAreDone(std::chrono::milliseconds timeout = std::chrono::seconds(5));

	enum class State : uint8_t { Running, Stopping, Stopped };
	std::atomic<State> state {State::Running};
	moodycamel::ConcurrentQueue<TaskSpec> taskQueue;
	std::list<std::shared_ptr<Task>> tasks;
	std::future<void> fut;
	std::atomic<bool> synchronous {false};
	std::atomic<bool> shouldStop {false};
	std::string name;
};
