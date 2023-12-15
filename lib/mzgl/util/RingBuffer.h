//
//  RingBuffer.h
//  mzgl
//
//  Created by Marek Bereza on 06/03/2023.
//  Copyright © 2023 Marek Bereza. All rights reserved.
//

#pragma once

/**
 * This is a bit different to circular buffer - its for floats only
 * and its very specific about whether an item can be pushed or popped.
 */
class RingBuffer {
public:
	RingBuffer(std::size_t size = 3) { reserve(size); }

	void reserve(std::size_t sz) { data.resize(sz); }

	bool isFull() const { return numItems == data.size(); }
	bool isEmpty() const { return numItems == 0; }

	bool push(float val) {
		if (isFull()) return false;
		data[tail] = val;
		tail	   = (tail + 1) % data.size();
		++numItems;
		return true;
	}

	bool pop(float &outVal) {
		if (isEmpty()) return false;
		outVal = data[head];
		head   = (head + 1) % data.size();
		--numItems;
		return true;
	}

	float &operator[](int i) { return data[(head + i) % data.size()]; }

	std::size_t size() const { return numItems; }

private:
	int head			 = 0;
	int tail			 = 0;
	std::size_t numItems = 0;
	std::vector<float> data;
};
