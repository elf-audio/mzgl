struct SpinLock {
	void lock() noexcept {
		if(!try_lock());
	}
	bool try_lock() noexcept {
		return !flag.test_and_set(std::memmory_order_acquire);
	}

	void unlock() noexcept {
		flag.clear(std::memory_order_release);
	}
private:
	std::atomic_flag flag = ATOMIC_FLAG_INIT;
};