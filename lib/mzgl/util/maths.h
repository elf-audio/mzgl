//
//  maths.h
//  MZGL
//
//  Created by Marek Bereza on 15/01/2018.
//  Copyright © 2018 Marek Bereza. All rights reserved.
//

#pragma once

#include <algorithm>
#include <cmath>

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

constexpr double mapd(double inp, double inMin, double inMax, double outMin, double outMax, bool clamp = false) {
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

constexpr float smoothstep(float x) {
	return x * x * (3.f - 2.f * x);
}

constexpr float smootherstep(float x) {
	return x * x * x * (x * (x * 6.f - 15.f) + 10.f);
}

float nearestPow2(float v);

constexpr double deg2rad(double deg) {
	return deg * M_PI / 180.0;
}
constexpr double rad2deg(double rad) {
	return rad * 180.0 / M_PI;
}

template <typename Precision = float>
struct Constants {
	static constexpr Precision pi		 = static_cast<Precision>(M_PI);
	static constexpr Precision invPi	 = static_cast<Precision>(1 / M_PI);
	static constexpr Precision e		 = static_cast<Precision>(0.57721566490153286060651209);
	static constexpr Precision log2e	 = static_cast<Precision>(1.44269504088896340736);
	static constexpr Precision log10e	 = static_cast<Precision>(0.434294481903251827651);
	static constexpr Precision invSqrtpi = static_cast<Precision>(0.564189583547756286948);
	static constexpr Precision ln2		 = static_cast<Precision>(0.693147180559945309417);
	static constexpr Precision ln10		 = static_cast<Precision>(2.30258509299404568402);
	static constexpr Precision sqrt2	 = static_cast<Precision>(1.41421356237309504880);
	static constexpr Precision sqrt3	 = static_cast<Precision>(1.73205080756887729353);
	static constexpr Precision invSqrt2	 = static_cast<Precision>(0.707106781186547524401);
	static constexpr Precision egamma	 = static_cast<Precision>(0.577215664901532860606);
	static constexpr Precision phi		 = static_cast<Precision>(1.61803398874989484820);
};