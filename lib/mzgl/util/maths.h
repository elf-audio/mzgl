//
//  maths.h
//  MZGL
//
//  Created by Marek Bereza on 15/01/2018.
//  Copyright © 2018 Marek Bereza. All rights reserved.
//

#pragma once

/**
 * TODO: get rid of clampf and replace with std::clamp
 */

constexpr inline float clampf(float inp, float from, float to) {
	if(inp < from) return from;
	if(inp>to) return to;
	return inp;
}

constexpr inline float mapf(float inp, float inMin, float inMax, float outMin, float outMax, bool clamp = false) {
	
	const float f = outMin + (outMax - outMin) * (inp - inMin) / (inMax - inMin);
	if(clamp) {
		if(outMax > outMin) {
			return clampf(f, outMin, outMax);
		} else {
			return clampf(f, outMax, outMin);
		}
	}
	return f;
}

double mapd(double inp, double inMin, double inMax, double outMin, double outMax, bool clamp = false);

double clampd(double inp, double from, double to);


int clampi(int inp, int from, int to);
long clampl(long inp, long from, long to);


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
