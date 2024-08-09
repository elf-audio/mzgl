// from here: https://github.com/progschj/ThreadPool/blob/master/ThreadPool.h

#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <vector>
#include <queue>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include <stdexcept>
#include "util.h"

class ThreadPool {
public:
	ThreadPool(size_t);
	~ThreadPool();

	template <class F, class... Args>
	auto enqueue(F &&f, Args &&...args) -> std::future<typename std::invoke_result<F, Args...>::type>;

	void stop();

private:
	// need to keep track of threads so we can join them
	std::vector<std::thread> workers;
	// the task queue
	std::queue<std::function<void()>> tasks;

	// synchronization
	std::mutex queue_mutex;
	std::condition_variable condition;
	std::atomic<bool> stopped {false};
};

inline ThreadPool::ThreadPool(size_t threads) {
	for (size_t i = 0; i < threads; ++i)
		workers.emplace_back([this, i] {
			setThreadName("ThreadPool worker " + std::to_string(i));

			for (;;) {
				std::function<void()> task;

				{
					std::unique_lock<std::mutex> lock(this->queue_mutex);
					condition.wait(lock, [this] { return stopped.load() || !this->tasks.empty(); });
					if (stopped && tasks.empty()) return;
					task = std::move(tasks.front());
					tasks.pop();
				}

				task();
			}
		});
}

// add new work item to the pool
template <class F, class... Args>
auto ThreadPool::enqueue(F &&f, Args &&...args) -> std::future<typename std::invoke_result<F, Args...>::type> {
	using return_type = typename std::invoke_result<F, Args...>::type;
	auto task		  = std::make_shared<std::packaged_task<return_type()>>(
		std::bind(std::forward<F>(f), std::forward<Args>(args)...));

	std::future<return_type> res = task->get_future();
	{
		std::unique_lock<std::mutex> lock(queue_mutex);

		// don't allow enqueueing after stopping the pool
		if (stopped.load()) {
			throw std::runtime_error("enqueue on stopped ThreadPool");
		}

		tasks.emplace([task]() { (*task)(); });
	}
	condition.notify_one();
	return res;
}

inline ThreadPool::~ThreadPool() {
	stop();
}

inline void ThreadPool::stop() {
	if (stopped.load()) {
		return;
	}

	stopped.store(true);
	condition.notify_all();

	for (std::thread &worker: workers) {
		if (worker.joinable()) {
			worker.join();
		}
	}
}

#endif
