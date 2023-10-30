//
//  CircularBuffer.h
//  koala
//
//  Created by Marek Bereza on 09/02/2021.
//  Copyright © 2021 Marek Bereza. All rights reserved.
//

#pragma once

#include <string.h>

/**
 * This class is missing a lot of things
 * but works for what koala needs it for.
 * It doesn't check if you try to consume
 * too many items, or insert too many.
 *
 * Also, it's not to be used for concurrency.
 */
template <typename T>
class CircularBuffer {
public:
	CircularBuffer(size_t sz = 3) { reserve(sz); }

	void reserve(size_t sz) { buff.resize(sz + 1); }
	size_t capacity() { return buff.size() - 1; }

	// REWRITE THESE 2 METHODS:
	// was insert
	void write(const T *d, size_t len) {
		if (writePos + len < buff.size()) {
			// no wrap
			memcpy(buff.data() + writePos + 1, d, len * sizeof(T));
			writePos = (writePos + len) % buff.size();
		} else {
			// wrap
			size_t toEnd = (buff.size() - writePos - 1);
			memcpy(buff.data() + writePos + 1, d, toEnd * sizeof(T));
			memcpy(buff.data(), d + toEnd, (len - toEnd) * sizeof(T));
			writePos = len - toEnd - 1;
		}
	}

	// was consume
	void read(T *d, size_t len) {
		for (int i = 0; i < len; i++) {
			d[i] = read();
		}
	}

	T &operator[](int i) { return buff[(readPos + i + 1) % buff.size()]; }

	void readMonoToStereo(float *d, int numFrames) {
		for (int i = 0; i < numFrames; i++) {
			d[i * 2] = d[i * 2 + 1] = read();
		}
	}

	// was insert
	void write(T a) {
		writePos = (writePos + 1) % buff.size();
		buff[writePos] = a;
	}
	// was consume
	T read() {
		readPos = (readPos + 1) % buff.size();
		return buff[readPos];
	}

	T peek() { return buff[(readPos + 1) % buff.size()]; }

	size_t size() const {
		if (writePos < readPos) {
			return buff.size() - readPos + writePos;
		} else {
			return writePos - readPos;
		}
	}

	void clear() { readPos = writePos; }

	// was insert
	void write(const std::vector<T> &items) { write(items.data(), (int) items.size()); }

	// was consume
	void read(std::vector<T> &out) { read(out.data(), (int) out.size()); }
	// was consume
	void readMonoToStereo(std::vector<T> &out) { readMonoToStereo(out.data(), out.size() / 2); }

private:
	std::vector<T> buff;

	size_t writePos = 0;
	size_t readPos = 0;
};
