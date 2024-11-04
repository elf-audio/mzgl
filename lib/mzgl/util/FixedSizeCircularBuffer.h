#pragma once
#include <array>
#include <cstddef>

template <typename T, size_t N>
class FixedSizeCircularBuffer {
public:
	FixedSizeCircularBuffer()
		: headPosition(0)
		, currentSize(0) {}

	void push(const T &item) {
		buffer[headPosition] = item;
		headPosition		 = (headPosition + 1) % N;
		if (currentSize < N) {
			++currentSize;
		}
	}

	T &operator[](size_t index) {
		if (index >= currentSize) {
			throw std::out_of_range("Index out of range");
		}
		size_t idx = (headPosition + N - currentSize + index) % N;
		return buffer[idx];
	}

	const T &operator[](size_t index) const {
		if (index >= currentSize) {
			throw std::out_of_range("Index out of range");
		}
		size_t idx = (headPosition + N - currentSize + index) % N;
		return buffer[idx];
	}

	[[nodiscard]] size_t size() const { return currentSize; }
	[[nodiscard]] bool empty() const { return size() == 0; }

	void clear() {
		headPosition = 0;
		currentSize	 = 0;
	}

	class iterator {
	public:
		using iterator_category = std::random_access_iterator_tag;
		using value_type		= T;
		using difference_type	= std::ptrdiff_t;
		using pointer			= T *;
		using reference			= T &;

		iterator(FixedSizeCircularBuffer *_buffer, std::size_t _index)
			: buffer(_buffer)
			, index(_index) {}

		reference operator*() const { return (*buffer)[index]; }
		pointer operator->() const { return &(*buffer)[index]; }

		iterator &operator++() {
			++index;
			return *this;
		}

		iterator operator++(int) {
			iterator tmp = *this;
			++index;
			return tmp;
		}

		iterator &operator--() {
			--index;
			return *this;
		}

		iterator operator--(int) {
			iterator tmp = *this;
			--index;
			return tmp;
		}

		iterator operator+(difference_type n) const { return iterator(buffer, index + n); }
		iterator operator-(difference_type n) const { return iterator(buffer, index - n); }

		difference_type operator-(const iterator &other) const {
			return static_cast<difference_type>(index) - static_cast<difference_type>(other.index);
		}

		iterator &operator+=(difference_type n) {
			index += n;
			return *this;
		}

		iterator &operator-=(difference_type n) {
			index -= n;
			return *this;
		}

		bool operator==(const iterator &other) const { return buffer == other.buffer && index == other.index; }
		bool operator!=(const iterator &other) const { return !(*this == other); }
		bool operator<(const iterator &other) const { return index < other.index; }
		bool operator>(const iterator &other) const { return index > other.index; }
		bool operator<=(const iterator &other) const { return index <= other.index; }
		bool operator>=(const iterator &other) const { return index >= other.index; }

	private:
		FixedSizeCircularBuffer *buffer;
		std::size_t index;
	};

	class const_iterator {
	public:
		using iterator_category = std::random_access_iterator_tag;
		using value_type		= const T;
		using difference_type	= std::ptrdiff_t;
		using pointer			= const T *;
		using reference			= const T &;

		const_iterator(const FixedSizeCircularBuffer *_buffer, std::size_t _index)
			: buffer(_buffer)
			, index(_index) {}

		reference operator*() const { return (*buffer)[index]; }
		pointer operator->() const { return &(*buffer)[index]; }

		const_iterator &operator++() {
			++index;
			return *this;
		}

		const_iterator operator++(int) {
			const_iterator tmp = *this;
			++index;
			return tmp;
		}

		const_iterator &operator--() {
			--index;
			return *this;
		}

		const_iterator operator--(int) {
			const_iterator tmp = *this;
			--index;
			return tmp;
		}

		const_iterator operator+(difference_type n) const { return const_iterator(buffer, index + n); }
		const_iterator operator-(difference_type n) const { return const_iterator(buffer, index - n); }

		difference_type operator-(const const_iterator &other) const {
			return static_cast<difference_type>(index) - static_cast<difference_type>(other.index);
		}

		bool operator==(const const_iterator &other) const {
			return buffer == other.buffer && index == other.index;
		}

		const_iterator &operator+=(difference_type n) {
			index += n;
			return *this;
		}

		const_iterator &operator-=(difference_type n) {
			index -= n;
			return *this;
		}

		bool operator!=(const const_iterator &other) const { return !(*this == other); }
		bool operator<(const const_iterator &other) const { return index < other.index; }
		bool operator>(const const_iterator &other) const { return index > other.index; }
		bool operator<=(const const_iterator &other) const { return index <= other.index; }
		bool operator>=(const const_iterator &other) const { return index >= other.index; }

	private:
		const FixedSizeCircularBuffer *buffer;
		std::size_t index;
	};

	const_iterator cbegin() const { return const_iterator(this, 0); }
	const_iterator begin() const { return const_iterator(this, 0); }
	iterator begin() { return iterator(this, 0); }

	const_iterator cend() const { return const_iterator(this, currentSize); }
	const_iterator end() const { return const_iterator(this, currentSize); }
	iterator end() { return iterator(this, currentSize); }

private:
	std::array<T, N> buffer;
	size_t headPosition;
	size_t currentSize;
};