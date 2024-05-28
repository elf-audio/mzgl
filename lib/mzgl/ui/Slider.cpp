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
Slider &Slider::darkTheme() {
	color		= hexColor(0x474747);
	railColor	= hexColor(0x232323);
	handleColor = hexColor(0xb2b2b2);
	return *this;
}

void Slider::setValue(float f) {
	drag(mapf(f, min, max, x + height / 2, x + width - height / 2));
}

Slider::Slider(Graphics &g, const std::string &name, float &value, float min, float max, float curve)
	: Layer(g, name) {
	interactive = true;
	this->value = &value;
	this->min	= min;
	this->max	= max;
	this->curve = curve;
}

static std::string str(float val) {
	if (std::abs(val) < 1) return toSigFigs(val, 2);
	if (std::abs(val) < 20) return to_string(val, 1);
	return to_string(val, 0);
}

void Slider::draw() {
	g.setColor(railColor);
	g.drawRect(*this);
	g.setColor(handleColor);
	float handleWidth = height;
	float normVal	  = mapf(*value, min, max, 0, 1, true);
	float val		  = powf(normVal, 1.0f / curve) * (width - height) + height / 2;
	g.drawRect(x + val - handleWidth / 2, y, handleWidth, height);
	if (!name.empty()) {
		g.setColor(0);
		g.drawText(name + ": " + str(*value), x + 5, y + height - 5);
	}
}
void Slider::drag(float x) {
	float handleWidth = height;
	float normVal	  = mapf(x, handleWidth / 2, width - handleWidth / 2, 0, 1, true);
	float val		  = powf(normVal, curve) * (max - min) + min;

	if (value != nullptr) {
		*value = val;
	}

	if (valueChanged) {
		valueChanged(val);
	}
}

bool Slider::touchDown(float x, float y, int id) {
	if (inside(x, y)) {
		drag(x);
		return true;
	}
	return false;
}

void Slider::touchMoved(float x, float y, int id) {
	drag(x);
}
