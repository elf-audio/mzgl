//
//  events.h
//  MZGL
//
//  Created by Marek Bereza on 23/01/2018.
//  Copyright © 2018 Marek Bereza. All rights reserved.
//

#pragma once
#include <stdio.h>
#include <functional>

enum {
	UPDATE,
	DRAW,
	TOUCH_MOVED
};

template<class T>
void addListener(int type, T* const object, void(T::* const mf)(float,float,int));

void addListener(int type, void *obj, std::function<void()>);
void removeListener(int type, void *obj);

void callUpdateListeners();
void callDrawListeners();


void printListenerUsageCount();
