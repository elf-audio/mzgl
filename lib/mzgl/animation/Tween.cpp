
//
//  Tween.h
//  mySketch
//
//  Created by Marek on 1/25/15.
//
//

#include "Tween.h"
#include "events.h"
#include "maths.h"
#include "util.h"

using namespace std;

template <class T>
void Tween_<T>::update(float t) {
	if (!running) return;
	float v	  = mapf(t, startTime, endTime, 0, 1, true);
	v		  = ease(v);
	*valuePtr = v * (to - from) + from; //ofMap(v, 0, 1, from, to, true);

	if (t > endTime) {
		running = false;
		if (tweenComplete) {
			tweenComplete();
		}
		//		removeListener(UPDATE, this);
	}
}

template <class T>
float Tween_<T>::ease(float v) {
	switch (type) {
		case EaseType::EASE_OUT_CUBIC: return easeOutCubic(v);
		case EaseType::EASE_IN_CUBIC: return easeInCubic(v);
		case EaseType::EASE_IN_OUT_CUBIC: return easeInOutCubic(v);
		case EaseType::EASE_LINEAR:
		default: return v;
	}
}

void AnimationManager::tweenTo(float &val, float to, float duration, EaseType easing, float delay) {
	auto t = std::make_shared<Tween>();

	t->tweenTo(val, to, duration, easing, delay);
	animations.push_back(t);
	cleanUpCompletedAnimations();
}

void AnimationManager::tweenTo(glm::vec2 &val, glm::vec2 to, float duration, EaseType easing, float delay) {
	tweenTo(val.x, to.x, duration, easing, delay);
	tweenTo(val.y, to.y, duration, easing, delay);

	cleanUpCompletedAnimations();
}

void AnimationManager::tweenTo(glm::vec3 &val, glm::vec3 to, float duration, EaseType easing, float delay) {
	tweenTo(val.x, to.x, duration, easing, delay);
	tweenTo(val.y, to.y, duration, easing, delay);
	tweenTo(val.z, to.z, duration, easing, delay);

	cleanUpCompletedAnimations();
}

void AnimationManager::tweenTo(Rectf &val, Rectf to, float duration, EaseType easing, float delay) {
	tweenTo(val.x, to.x, duration, easing, delay);
	tweenTo(val.y, to.y, duration, easing, delay);
	tweenTo(val.width, to.width, duration, easing, delay);
	tweenTo(val.height, to.height, duration, easing, delay);

	cleanUpCompletedAnimations();
}

void AnimationManager::tweenTo(glm::vec4 &val, glm::vec4 to, float duration, EaseType easing, float delay) {
	tweenTo(val.r, to.r, duration, easing, delay);
	tweenTo(val.g, to.g, duration, easing, delay);
	tweenTo(val.b, to.b, duration, easing, delay);
	tweenTo(val.a, to.a, duration, easing, delay);

	cleanUpCompletedAnimations();
}

void AnimationManager::animate(float duration,
							   function<void(float)> progressFunc,
							   function<void()> completionFunc) {
	auto fa = std::make_shared<FunctionAnimation>(duration, progressFunc, completionFunc);
	animations.push_back(fa);
	cleanUpCompletedAnimations();
}
void AnimationManager::cleanUpCompletedAnimations() {
	for (int i = 0; i < animations.size(); i++) {
		if (!animations[i]->isRunning()) {
			animations.erase(animations.begin() + i);
			i--;
		}
	}
	//	printf("Num tweens on the go: %d\n", animations.size());
}

void AnimationManager::update() {
	if (!animations.empty()) {
		float t = getSeconds();

		for (auto &a: animations) {
			a->update(t);
		}
	}
}

FunctionAnimation::FunctionAnimation(float duration,
									 function<void(float)> progressFunc,
									 function<void()> completionFunc)
	: duration(duration)
	, progressFunc(progressFunc)
	, completionFunc(completionFunc) {
	running	  = true;
	startTime = getSeconds();
	endTime	  = startTime + duration;
}
void FunctionAnimation::update(float t) {
	if (!running) return;

	float v = mapf(t, startTime, endTime, 0, 1, true);
	progressFunc(v);
	if (t > endTime) {
		completionFunc();
		running = false;
		removeListener(UPDATE, this);
	}
}
FunctionAnimation::~FunctionAnimation() {
	if (running) {
		printf("ERROR! FunctionAnimation was not allowed to complete before deleting\n");
		completionFunc();
		removeListener(UPDATE, this);
	}
}
