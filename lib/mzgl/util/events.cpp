//
//  events.cpp
//  MZGL
//
//  Created by Marek Bereza on 23/01/2018.
//  Copyright © 2018 Marek Bereza. All rights reserved.
//

#include "events.h"

#include <stdio.h>
#include <vector>
#include <functional>

using namespace std;

vector<pair<void*, function<void()>>> updateListeners;
vector<pair<void*, function<void()>>> drawListeners;

void printListenerUsageCount() {
	printf("%d updateListeners\n", (int)updateListeners.size());
	printf("%d drawListeners\n", (int)drawListeners.size());
}

void addListener(int type, void *obj, function<void()> fn) {
	//printf("add Not implemented\n");
	switch(type) {
		case UPDATE:
			updateListeners.push_back(make_pair(obj, fn));
			break;
		case DRAW:
			drawListeners.push_back(make_pair(obj, fn));
			break;
	}
	
	
}
void removeFromListeners(vector<pair<void*, function<void()>>> &listeners, void *obj) {
	for(int i = 0; i < listeners.size(); i++) {
		if(listeners[i].first==obj) {
			listeners.erase(listeners.begin() + i);
			return;
		}
	}
}

void removeListener(int type, void *obj) {

	switch(type) {
		case UPDATE:
			removeFromListeners(updateListeners, obj);
			break;
		case DRAW:
			removeFromListeners(drawListeners, obj);
			break;
	}
}

void callUpdateListeners() {
	//for(auto &u : updateListeners) { // this crashes on windows??
    for(int i = 0; i < updateListeners.size(); i++) {
        updateListeners[i].second();
	//	u.second();
	}
}

void callDrawListeners() {
	for(auto &u : drawListeners) {
		u.second();
	}
}

std::vector<std::function<void(bool,int)>> touchMovedListeners;


template<class T>
void addListener(int type, T* const object, void(T::* const mf)(float,float,int)) {
	if(type==TOUCH_MOVED) {
		using namespace std::placeholders;
		touchMovedListeners.emplace_back(std::bind(mf, object, _1, _2, _3));
	} else {
		printf("ERROR: Can't do any events other than TOUCH_MOVED\n");
	}
}


