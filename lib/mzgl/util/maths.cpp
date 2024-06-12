//
//  maths.cpp
//  MZGL
//
//  Created by Marek Bereza on 15/01/2018.
//  Copyright © 2018 Marek Bereza. All rights reserved.
//

#include "maths.h"
#include <stdlib.h>
#include <math.h>

double mapd(double inp, double inMin, double inMax, double outMin, double outMax, bool clamp) {
	double norm = (inp - inMin) / (inMax - inMin);
	double f	= outMin + (outMax - outMin) * norm;
	if (clamp) {
		if (outMax > outMin) {
			return std::clamp(f, outMin, outMax);
		} else {
			return std::clamp(f, outMax, outMin);
		}
	}
	return f;
}

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
	return powf(10.f, db / 20.f);
}

float smoothstep(float x) {
	return x * x * (3.f - 2.f * x);
}

float smootherstep(float x) {
	return x * x * x * (x * (x * 6.f - 15.f) + 10.f);
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
