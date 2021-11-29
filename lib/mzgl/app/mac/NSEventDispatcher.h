//
//  NSEventDispatcher.h
//  geothing
//
//  Created by Marek Bereza on 20/03/2020.
//  Copyright © 2020 Marek Bereza. All rights reserved.
//

#pragma once

#include <vector>
#include <functional>
#include <AppKit/AppKit.h>

/*
 At the moment this is only in here for imgui
 */
class NSEventDispatcher {
public:
	
	static NSEventDispatcher &instance() {
		static NSEventDispatcher *inst = nullptr;
		if(inst==nullptr) {
			inst = new NSEventDispatcher();
		}
		return *inst;
	}
	
	void dispatch(NSEvent *evt, NSView *view) {
		for(auto &l : listeners) {
			l(evt, view);
		}
	}
	
	void addListener(std::function<void(NSEvent*,NSView*)> listener) {
		listeners.push_back(listener);
	}
	
	void clearListeners() {
		listeners.clear();
	}
private:
	std::vector<std::function<void(NSEvent*,NSView*)>> listeners;
	NSEventDispatcher() {}
	
};
