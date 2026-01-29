//
//  Slider.h
//
//
//

#pragma once
#include "Layer.h"
#include <functional>

class Slider : public Layer {
public:
	float *value;

	float minValue = 0;
	float maxValue = 1;

	float power = 1;

	void setValue(float f);
	Slider(Graphics &g, const std::string &name, float &value, float min = 0, float max = 1, float curve = 1);
	Slider(Graphics &g, float &value, float min = 0, float max = 1)
		: Slider(g, "", value, min, max) {}

	std::function<void(float)> valueChanged;

	bool touchDown(float x, float y, int id) override;
	void touchMoved(float x, float y, int id) override;
	void draw() override;

private:
	float valueToNormalized(float x);
	float normalizedToValue(float val);

	float initialValue = 0;
	float lastTapTime  = 0;
	static constexpr float doubleTapInterval = 0.35f;
};
