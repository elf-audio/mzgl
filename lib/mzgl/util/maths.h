//
//  maths.h
//  MZGL
//
//  Created by Marek Bereza on 15/01/2018.
//  Copyright © 2018 Marek Bereza. All rights reserved.
//

#pragma once
#include <algorithm>
constexpr inline float mapf(float inp, float inMin, float inMax, float outMin, float outMax, bool clamp = false) {
	const float f = outMin + (outMax - outMin) * (inp - inMin) / (inMax - inMin);
	if (clamp) {
		if (outMax > outMin) {
			return std::clamp(f, outMin, outMax);
		} else {
			return std::clamp(f, outMax, outMin);
		}
	}
	return f;
}

double mapd(double inp, double inMin, double inMax, double outMin, double outMax, bool clamp = false);

float nearestPow2(float v);

// random [0,1]
float randuf();

// random [-1,1]
float randf();
float randf(float to);
float randf(float from, float to);
int randi(int to);
int randi(int from, int to);
bool randb();
float rms2db(float inp);
float db2rms(float db);

float smoothstep(float x);
float smootherstep(float x);

double deg2rad(double deg);
double rad2deg(double rad);