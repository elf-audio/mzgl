//
//  CircularBuffer.h
//  koala
//
//  Created by Marek Bereza on 09/02/2021.
//  Copyright © 2021 Marek Bereza. All rights reserved.
//

#pragma once

#include <string.h>

template <typename T>
class CircularBuffer {
public:
    CircularBuffer() = default;
    CircularBuffer(size_t sz) {
        reserve(sz);
    }
    
	void reserve(size_t sz) {
		buff.resize(sz+1);
	}

	// REWRITE THESE 2 METHODS:
	void insert(const T *d, int len) {
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

	void consume(T *d, int len) {
		for(int i = 0; i < len; i++) {
			d[i] = consume();
		}
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

	size_t size() {
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


