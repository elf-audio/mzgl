//
//  Slider.h
//
//
//

#pragma once
#include <mzgl/ui/Layer.h>
#include <functional>

class Slider : public Layer {
public:
	float *value;
	vec4 color;

	float min = 0;
	float max = 1;

	float curve = 1;

	Slider &darkTheme();

	void setValue(float f);
	Slider(Graphics &g, const std::string &name, float &value, float min = 0, float max = 1, float curve = 1);
	Slider(Graphics &g, float &value, float min = 0, float max = 1)
		: Slider(g, "", value, min, max) {}

	std::function<void(float)> valueChanged;

	bool touchDown(float x, float y, int id) override;
	void touchMoved(float x, float y, int id) override;
	void draw() override;
	vec4 railColor {0.6, 0.6, 0.6, 1};
	vec4 handleColor = hexColor(0xCCCCCC);

private:
	void drag(float x);
};
