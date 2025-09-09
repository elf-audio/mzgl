#include "TaskRunner.h"
#include "util.h"

TaskRunner::TaskRunner(const std::string &_name, const bool _synchronous)
	: name(_name)
	, synchronous {_synchronous} {
	if (!synchronous.load()) {
		createFuture();
	}
}

TaskRunner::~TaskRunner() {
	requestStop(250ms);
}

void TaskRunner::createFuture() {
	fut = std::async(std::launch::async, [this]() {
		setThreadName(this->name);

		TaskSpec spec;
		while (!shouldStop.load(std::memory_order_acquire)) {
			clearAnyDoneTasks();

			while (taskQueue.try_dequeue(spec)) {
				tasks.emplace_back(std::make_unique<Task>(std::move(spec)));
			}
			std::this_thread::sleep_for(std::chrono::microseconds(100));
		}

		while (taskQueue.try_dequeue(spec)) {
			tasks.emplace_back(std::make_unique<Task>(std::move(spec)));
		}
	});
}

void TaskRunner::run(TaskRunner::TaskSpec spec) {
	if (synchronous.load()) {
		spec.task();
		if (spec.onDone) {
			spec.onDone();
		}
		return;
	}

	if (state.load(std::memory_order_acquire) != State::Running) {
		Log::w() << "TaskRunner '" << name << "' rejecting task; stopping";
		return;
	}

	taskQueue.enqueue(std::move(spec));
}

void TaskRunner::requestStop(std::chrono::milliseconds timeout) {
	if (synchronous.load()) {
		return;
	}

	auto prev = state.exchange(State::Stopping, std::memory_order_acq_rel);
	if (prev == State::Stopped) {
		return;
	}

	shouldStop.store(true, std::memory_order_release);
	fut.wait();

	waitTilAllTasksAreDone(timeout);
	state.store(State::Stopped, std::memory_order_release);
}

void TaskRunner::clearAnyDoneTasks() {
	tasks.remove_if([](const std::shared_ptr<Task> &task) { return task->done(); });
}

void TaskRunner::waitTilAllTasksAreDone(std::chrono::milliseconds timeout) {
	const auto deadline = std::chrono::steady_clock::now() + timeout;
	for (;;) {
		clearAnyDoneTasks();
		if (tasks.empty()) {
			return;
		}

		if (std::chrono::steady_clock::now() >= deadline) {
			size_t stuck = 0;
			for (auto &t: tasks) {
				if (!t->done()) {
					++stuck;
				}
			}
			Log::e() << "TaskRunner '" << name << "' timed out waiting for " << stuck << " task(s) to finish";
			return;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(5));
	}
}

TaskRunner::Task::Task(TaskSpec spec) {
	taskFuture = std::async(std::launch::async, [spec = std::move(spec)]() {
#if (DEBUG || UNIT_TEST)
		setThreadName("TaskRunner::Task");
#endif
		try {
			if (spec.task) {
				spec.task();
			}
		} catch (const std::exception &err) {
			std::string ex = err.what();
			Log::e() << "exception in runTask: " << ex;
		} catch (...) {
			Log::e() << "exception in runTask (unknown)";
		}

		if (spec.onDone) {
			try {
				spec.onDone();
			} catch (const std::exception &err) {
				Log::e() << "onDone exception: " << err.what();
			} catch (...) {
				Log::e() << "onDone exception (unknown)";
			}
		}
	});
}

bool TaskRunner::Task::done() {
	return taskFuture.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
}
