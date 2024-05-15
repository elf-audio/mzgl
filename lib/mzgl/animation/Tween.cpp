
//
//  Tween.h
//  mySketch
//
//  Created by Marek on 1/25/15.
//
//

#include "Tween.h"
#include "maths.h"
#include "util.h"

using namespace std;

template <class T>
void Tween_<T>::update(float t) {
	if (!running) return;
	float v	  = mapf(t, startTime, endTime, 0, 1, true);
	v		  = ease(v);
	*valuePtr = v * (to - from) + from;

	if (t > endTime) {
		running = false;
		if (tweenComplete) {
			tweenComplete();
		}
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

class Timeout : public Animation {
public:
	Timeout(float delayDuration, std::function<void()> theCallback)
		: callback(theCallback) {
		endTime = getSeconds() + delayDuration;
	}

	bool isRunning() override { return !hasCalledCallback; }

	void update(float currTime) override {
		if (currTime >= endTime && !hasCalledCallback) {
			callback();
			hasCalledCallback = true;
		}
	}

private:
	bool hasCalledCallback = false;
	std::function<void()> callback;
	float endTime;
};

void AnimationManager::setTimeout(float duration, std::function<void()> callback) {
	animations.push_back(std::make_shared<Timeout>(duration, callback));
	cleanUpCompletedAnimations();
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

void AnimationManager::cleanUpCompletedAnimations() {
	for (int i = 0; i < animations.size(); i++) {
		if (!animations[i]->isRunning()) {
			animations.erase(animations.begin() + i);
			i--;
		}
	}
}

void AnimationManager::update(double currFrameTime) {
	for (auto &a: animations) {
		a->update(currFrameTime);
	}
}
