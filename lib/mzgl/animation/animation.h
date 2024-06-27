/**     ___           ___           ___                         ___           ___     
 *     /__/\         /  /\         /  /\         _____         /  /\         /__/|    
 *    |  |::\       /  /::\       /  /::|       /  /::\       /  /::\       |  |:|    
 *    |  |:|:\     /  /:/\:\     /  /:/:|      /  /:/\:\     /  /:/\:\      |  |:|    
 *  __|__|:|\:\   /  /:/~/::\   /  /:/|:|__   /  /:/~/::\   /  /:/  \:\   __|__|:|    
 * /__/::::| \:\ /__/:/ /:/\:\ /__/:/ |:| /\ /__/:/ /:/\:| /__/:/ \__\:\ /__/::::\____
 * \  \:\~~\__\/ \  \:\/:/__\/ \__\/  |:|/:/ \  \:\/:/~/:/ \  \:\ /  /:/    ~\~~\::::/
 *  \  \:\        \  \::/          |  |:/:/   \  \::/ /:/   \  \:\  /:/      |~~|:|~~ 
 *   \  \:\        \  \:\          |  |::/     \  \:\/:/     \  \:\/:/       |  |:|   
 *    \  \:\        \  \:\         |  |:/       \  \::/       \  \::/        |  |:|   
 *     \__\/         \__\/         |__|/         \__\/         \__\/         |__|/   
 *
 *  Description: 
 *				 
 *  animation.h, created by Marek Bereza on 25/04/2012.
 */

#pragma once
#include <cmath>

#ifndef M_PI
#	define M_PI 3.14159265358979323846
#endif

// b is offset
// d is total duration
// t is position
// c is distance
// so we want
//b = 0,
//d = 1,
//c = 1,
//t = [0,1]

inline float easeInBack(float t) {
	float s = 1.70158f;
	return t * t * ((s + 1) * t - s);
}

inline float easeOutBack(float t) {
	float s = 1.70158f;
	t--;
	return t * t * ((s + 1) * t + s) + 1;
}

inline float easeInOutBack(float t) {
	float s = 2.59491f;
	if ((t /= 0.5) < 1) return 1.f / 2 * (t * t * ((s + 1) * t - s));

	float postFix = t -= 2;
	return 1.f / 2 * ((postFix) *t * ((s + 1) * t + s) + 2);
}

inline float easeOutBounce(float t) {
	if ((t) < (1 / 2.75f)) {
		return (7.5625f * t * t);
	} else if (t < (2 / 2.75f)) {
		float postFix = t -= (1.5f / 2.75f);
		return (7.5625f * (postFix) *t + .75f);
	} else if (t < (2.5 / 2.75)) {
		float postFix = t -= (2.25f / 2.75f);
		return (7.5625f * (postFix) *t + .9375f);
	} else {
		float postFix = t -= (2.625f / 2.75f);
		return (7.5625f * (postFix) *t + .984375f);
	}
}

inline float easeInBounce(float t) {
	return 1 - easeOutBounce(1.f - t);
}
inline float easeInOutBounce(float t) {
	if (t < 1 / 2) return easeInBounce(t * 2) * .5f;
	else return easeOutBounce(t * 2 - 1) * .5f + .5f;
}

inline float easeInCirc(float t) {
	return -1 * (sqrt(1 - (t) *t) - 1);
}
inline float easeOutCirc(float t) {
	t--;
	return sqrt(1 - t * t);
}

inline float easeInOutCirc(float t) {
	if ((t /= 1.f / 2) < 1) return -1.f / 2 * (sqrt(1 - t * t) - 1);
	return 1.f / 2 * (sqrt(1 - t * (t - 2)) + 1);
}

inline float easeInCubic(float t) {
	return (t) *t * t;
}
inline float easeOutCubic(float t) {
	t--;
	return t * t * t + 1;
}

inline float easeInOutCubic(float t) {
	if ((t /= 1.f / 2) < 1) return 1.f / 2 * t * t * t;
	t -= 2;
	return 1.f / 2 * (t * t * t + 2);
}

inline float easeInElastic(float t) {
	if (t == 0) return 0;
	if (t == 1) return 1;
	float p = .3f;

	float s		  = p / 4;
	float postFix = pow(2.f, 10.f * (t -= 1.f)); // this is a fix, again, with post-increment operators
	return -(postFix * sin((t - s) * (2.f * M_PI) / p));
}

inline float easeOutElastic(float t) {
	if (t == 0) return 0;
	if ((t) == 1) return 1;
	float p = .3f;

	float s = p / 4;
	return (pow(2, -10 * t) * sin((t - s) * (2 * M_PI) / p) + 1);
}

inline float easeInOutElastic(float t) {
	if (t == 0) return 0;
	if ((t /= 0.5) == 2) return 1;
	float p = (.3f * 1.5f);

	float s = p / 4;

	if (t < 1) {
		float postFix = pow(2, 10 * (t -= 1)); // postIncrement is evil
		return -.5f * (postFix * sin((t - s) * (2 * M_PI) / p));
	}
	float postFix = pow(2, -10 * (t -= 1)); // postIncrement is evil
	return postFix * sin((t - s) * (2 * M_PI) / p) * .5f + 1;
}

inline float easeInExpo(float t) {
	return (t == 0) ? 0 : pow(2, 10 * (t - 1));
}
inline float easeOutExpo(float t) {
	return (t == 1) ? 1 : (-pow(2, -10 * t) + 1);
}

inline float easeInOutExpo(float t) {
	if (t == 0) return 0;
	if (t == 1) return 1;
	if ((t /= 1.f / 2) < 1) return 1.f / 2 * pow(2, 10 * (t - 1));
	return 1.f / 2 * (-pow(2, -10 * --t) + 2);
}

inline float easeInQuad(float t) {
	return t * t;
}
inline float easeOutQuad(float t) {
	return -1.f * (t) * (t - 2);
}

inline float easeInOutQuad(float t) {
	if ((t /= 1.f / 2) < 1) return 1.f / 2 * t * t;
	t--;
	return -1.f / 2 * (t * (t - 2) - 1);
}

inline float easeInQuart(float t) {
	return t * t * t * t;
}
inline float easeOutQuart(float t) {
	t--;
	return -1 * (t * t * t * t - 1);
}

inline float easeInOutQuart(float t) {
	if ((t /= 1.f / 2) < 1) return 1.f / 2 * t * t * t * t;
	t -= 2;
	return -1.f / 2 * (t * t * t * t - 2);
}

inline float easeInQuint(float t) {
	return t * t * t * t * t;
}
inline float easeOutQuint(float t) {
	t--;
	return (t * t * t * t * t + 1);
}

inline float easeInOutQuint(float t) {
	if ((t /= 1.f / 2) < 1) return 1.f / 2 * t * t * t * t * t;
	t -= 2;
	return 1.f / 2 * (t * t * t * t * t + 2);
}

inline float easeInSine(float t) {
	return -1 * cos(t * (M_PI / 2)) + 1;
}
inline float easeOutSine(float t) {
	return sin(t * (M_PI / 2));
}

inline float easeInOutSine(float t) {
	return -1.f / 2 * (cos(M_PI * t) - 1);
}
