//
//  Slider.cpp
//  TapeSampler
//
//  Created by Marek Bereza on 22/11/2017.
//
//

#include "Slider.h"
#include "maths.h"
#include "stringUtil.h"

static vec4 railColor {0.6, 0.6, 0.6, 1};
static vec4 handleColor = hexColor(0xCCCCCC);

void Slider::setValue(float f) {
	*value = f;
}

Slider::Slider(Graphics &g, const std::string &name, float &_value, float _min, float _max, float _power)
	: Layer(g, name)
	, minValue(_min)
	, maxValue(_max)
	, power(_power)
	, value(&_value)
	, initialValue(_value) {
	interactive = true;
}

static std::string str(float val) {
	if (std::abs(val) < 1) return toSigFigs(val, 2);
	if (std::abs(val) < 20) return to_string(val, 1);
	return to_string(val, 0);
}

float Slider::valueToNormalized(float x) {
	float val = mapf(x, minValue, maxValue, 0, 1, true);
	if (power != 1.f) {
		val = std::pow(val, 1.f / power);
	}
	return val;
}

float Slider::normalizedToValue(float val) {
	if (power != 1.f) {
		if (val > 1) val = 1;
		else if (val < 0) val = 0;
		val = std::pow(val, power);
	}
	return mapf(val, 0, 1, minValue, maxValue, true);
}

void Slider::draw() {
	g.fill();
	g.setColor(railColor);
	g.drawRect(*this);
	g.setColor(handleColor);

	auto norm = valueToNormalized(*value);
	g.drawRect(x, y, width * norm, height);
	if (!name.empty()) {
		g.setColor(0);
		g.drawText(name + ": " + str(*value), x + 5, y + height - 5);
	}
}

bool Slider::touchDown(float x, float y, int id) {
	if (!inside(x, y)) return false;

	float now = static_cast<float>(g.currFrameTime);
	if (now - lastTapTime < doubleTapInterval) {
		*value = initialValue;
		if (valueChanged) {
			valueChanged(initialValue);
		}
		lastTapTime = 0;
	} else {
		lastTapTime = now;
		touchMoved(x, y, id);
	}
	return true;
}

void Slider::touchMoved(float x, float y, int id) {
	float norm = std::clamp((x - this->x) / width, 0.f, 1.f);
	float val  = normalizedToValue(norm);
	if (val != *value) {
		*value = val;
		if (valueChanged) {
			valueChanged(val);
		}
	}
}
