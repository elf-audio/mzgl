//
//  maths.cpp
//  MZGL
//
//  Created by Marek Bereza on 15/01/2018.
//  Copyright © 2018 Marek Bereza. All rights reserved.
//

#include "maths.h"
#include <stdlib.h>

float randuf() {
	return rand() / (float) RAND_MAX;
}
float randf() {
	return randuf() * 2.f - 1.f;
}
float randf(float to) {
	return randuf() * to;
}

float randf(float from, float to) {
	return from + randuf() * (to - from);
}

int randi(int to) {
	return rand() % (to + 1);
}
int randi(int from, int to) {
	return from + rand() % (to + 1 - from);
}
bool randb() {
	return rand() % 2;
}

float rms2db(float inp) {
	return 20 * log10(inp);
}

float db2rms(float db) {
	return std::pow(10.f, db / 20.f);
}
float nearestPow2(float v) {
	float lo = floor(log2(v));
	float hi = ceil(log2(v));
	lo		 = pow(2.0, lo);
	hi		 = pow(2.0, hi);

	if (abs((int) (v - lo)) < abs((int) (v - hi))) {
		return lo;
	} else {
		return hi;
	}
}
