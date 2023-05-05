//
//  FlatSet.h
//  koala
//
//  Created by Marek Bereza on 06/12/2021.
//  Copyright © 2021 Marek Bereza. All rights reserved.
//

#pragma once
#include <vector>

/**
 If you need to use a set but want it to not allocate
 */
template<typename T>
class FlatSet {
public:
	
	FlatSet(size_t initialSize = 0) {
		reserve(initialSize);
	}
	
	void reserve(size_t sz) {
		storage.reserve(sz);
	}
	
	bool insert(T s) {
		// only insert if not already inside
		for(auto spd : storage) {
			if(spd==s) return false;
		}
		storage.push_back(s);
		return true;
	}
	
	bool contains(const T &s) const {
		for(auto spd : storage) {
			if(spd==s) return true;
		}
		return false;
	}
	
	bool erase(T p) {
		for(size_t i = 0; i < storage.size(); i++) {
			if(storage[i]==p) {
				storage.erase(storage.begin() + i);
				return true;
			}
		}
		return false;
	}
	
	bool empty() const  { return storage.empty(); }
	void clear()        {  storage.clear(); }
	size_t size() const { return storage.size(); }
	const T &operator[](int i) const {
		return storage[i];
	}

	const std::vector<T> &getVector() const { return storage; }
private:
	std::vector<T> storage;
};
