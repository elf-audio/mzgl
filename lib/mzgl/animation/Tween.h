//
//  Tween.h
//  mySketch
//
//  Created by Marek on 1/25/15.
//
//

#pragma once

#include "animation.h"
#include <functional>
#include "Rectf.h"
#include <vector>

#include <memory>

enum class EaseType {

	EASE_LINEAR,
	EASE_OUT_CUBIC,
	EASE_IN_CUBIC,
	EASE_IN_OUT_CUBIC
};
// base type for different types of animation moves
class Animation {
public:
	virtual bool isRunning()			= 0;
	virtual void update(float currTime) = 0;
	virtual ~Animation() {}
};

template <class T>
class Tween_ : public Animation {
public:
	T *valuePtr;

	std::function<void()> tweenComplete;
	T getValue() { return *valuePtr; }
	void tweenTo(T &valuePtr, T to, float duration, EaseType type = EaseType::EASE_LINEAR, float delay = 0);

	void easeInOutTo(T to, float duration) { tweenTo(*valuePtr, to, duration, EaseType::EASE_IN_OUT_CUBIC); }
	void start(T &valuePtr, T from, T to, float duration, EaseType type = EaseType::EASE_LINEAR, float delay = 0);

	bool isRunning() override { return running; }
	void pointTo(T &var) { valuePtr = &var; }
	void setToValue(T f) {
		to = f;
		if (isDone()) {
			*valuePtr = to;
		}
	}
	T getToValue() { return to; }
	bool isDone() { return !running && endTime != 0; }
	void update(float currTime) override;

private:
	float startTime = 0;
	float endTime	= 0;
	T from;
	T to;
	bool running = false;
	EaseType type;

	float ease(float v);
};

template <class T>
void Tween_<T>::tweenTo(T &valuePtr, T to, float duration, EaseType type, float delay) {
	start(valuePtr, valuePtr, to, duration, type, delay);
}

#include "util.h"

template <class T>
void Tween_<T>::start(T &valuePtr, T from, T to, float duration, EaseType type, float delay) {
	if (!running) {
		//		addListener(UPDATE, this, [this]() { update(); });
		running = true;
	}
	this->type		= type;
	this->valuePtr	= &valuePtr;
	this->from		= from;
	this->to		= to;
	this->startTime = getSeconds() + delay;
	this->endTime	= startTime + duration;
}

typedef Tween_<float> Tween;

// not sure if this works
//typedef Tween_<glm::vec2> Tween2f;

class FunctionAnimation : public Animation {
public:
	float startTime;
	float endTime;
	bool running = false;

	float duration;
	std::function<void(float)> progressFunc;
	std::function<void()> completionFunc;
	FunctionAnimation(float duration,
					  std::function<void(float)> progressFunc,
					  std::function<void()> completionFunc);

	bool isRunning() override { return running; }
	void update(float currTime) override;
	~FunctionAnimation();
};

class AnimationManager {
public:
	AnimationManager() {}

	void animate(float duration, std::function<void(float)> progressFunc, std::function<void()> completionFunc);
	void tweenTo(float &val, float to, float duration, EaseType easing = EaseType::EASE_LINEAR, float delay = 0);
	void tweenTo(
		glm::vec2 &val, glm::vec2 to, float duration, EaseType easing = EaseType::EASE_LINEAR, float delay = 0);
	void tweenTo(
		glm::vec3 &val, glm::vec3 to, float duration, EaseType easing = EaseType::EASE_LINEAR, float delay = 0);
	void tweenTo(
		glm::vec4 &val, glm::vec4 to, float duration, EaseType easing = EaseType::EASE_LINEAR, float delay = 0);

	void tweenTo(Rectf &val, Rectf to, float duration, EaseType easing = EaseType::EASE_LINEAR, float delay = 0);

	void easeInOut(float &var, float to, float duration) {
		return tweenTo(var, to, duration, EaseType::EASE_IN_OUT_CUBIC);
	}

	void easeInOut(Rectf &var, Rectf to, float duration) {
		return tweenTo(var, to, duration, EaseType::EASE_IN_OUT_CUBIC);
	}
	void update();

private:
	std::vector<std::shared_ptr<Animation>> animations;

public: // just for now to test functionality
	void cleanUpCompletedAnimations();
};

#define ANIMATE(duration, progressFunc, completionFunc)                                                           \
	AnimationManager::getInstance()->animate(duration, progressFunc, completionFunc)
#define TWEEN(var, to, duration, tween) AnimationManager::getInstance()->tweenTo(var, to, duration, tween)
#define EASE_IN(var, to, duration)		AnimationManager::getInstance()->tweenTo(var, to, duration, EASE_IN_CUBIC)
#define EASE_OUT(var, to, duration)		AnimationManager::getInstance()->tweenTo(var, to, duration, EASE_OUT_CUBIC)
#define EASE_IN_OUT(var, to, duration)                                                                            \
	AnimationManager::getInstance()->tweenTo(var, to, duration, EASE_IN_OUT_CUBIC)
#define EASE_IN_OUT_FN(var, to, duration, fn)                                                                     \
	AnimationManager::getInstance()->tweenTo(var, to, duration, EASE_IN_OUT_CUBIC, 0, fn)

#define EASE_IN_OUT_DELAY(var, to, duration, delay)                                                               \
	AnimationManager::getInstance()->tweenTo(var, to, duration, EASE_IN_OUT_CUBIC, delay)
