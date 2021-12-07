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
		storage.reserve(initialSize);
	}
	
	void insert(T s) {
		// only insert if not already inside
		for(auto spd : storage) {
			if(spd==s) return;
		}
		storage.push_back(s);
	}
	
	bool has(T *s) const {
		for(auto spd : storage) {
			if(spd.get()==s) return true;
		}
		return false;
	}
	
	void remove(T p) {
		for(size_t i = 0; i < storage.size(); i++) {
			if(storage[i]==p) {
				storage.erase(storage.begin() + i);
				return;
			}
		}
	}
	
	bool empty()  { return storage.empty(); }
	void clear()  { 	   storage.clear(); }
	size_t size() { return storage.size(); }
	
	std::vector<T> &getVector() { return storage; }
private:
	std::vector<T> storage;
};
