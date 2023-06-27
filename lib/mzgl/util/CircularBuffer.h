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

    CircularBuffer(size_t sz = 3) {
        reserve(sz);
    }
    
	void reserve(size_t sz) {
		buff.resize(sz+1);
	}

	// REWRITE THESE 2 METHODS:
	void insert(const T *d, size_t len) {
		if(writePos + len < buff.size()) {
			// no wrap
			memcpy(buff.data()+writePos+1,d, len*sizeof(T));
			writePos = (writePos + len) % buff.size();
		} else {
			// wrap
			size_t toEnd = (buff.size() - writePos - 1);
			memcpy(buff.data() + writePos + 1, d, toEnd*sizeof(T));
			memcpy(buff.data(), d + toEnd, (len - toEnd) * sizeof(T));
			writePos = len - toEnd - 1;
		}
	}

	void consume(T *d, size_t len) {
		for(int i = 0; i < len; i++) {
			d[i] = consume();
		}
	}
	
	float &operator[](int i) {
		return buff[(readPos + i+1) % buff.size()];
	}

	void consumeMonoToStereo(float *d, int numFrames) {
		for(int i =0 ; i < numFrames; i++) {
			d[i*2] = d[i*2+1] = consume();
		}
	}

	
	void insert(T a) {
		writePos = (writePos+1) % buff.size();
		buff[writePos] = a;
	}

	T consume() {
		readPos = (readPos+1) % buff.size();
		return buff[readPos];
	}

	T peek() {
		return buff[(readPos+1) % buff.size()];
	}
	
	size_t size() const {
		if(writePos<readPos) {
			return buff.size() - readPos + writePos;
		} else {
			return writePos - readPos;
		}
	}

	void clear() {
		readPos = writePos;
	}

	void insert(const std::vector<T> &items) {
		insert(items.data(), (int)items.size());
	}

	void consume(std::vector<T> &out) {
		consume(out.data(), (int)out.size());
	}
	void consumeMonoToStereo(std::vector<T> &out) {
		consumeMonoToStereo(out.data(), out.size()/2);
	}
    
private:
    std::vector<T> buff;
    
    size_t writePos = 0;
    size_t readPos = 0;
};


