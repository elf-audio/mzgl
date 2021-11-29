//
//  Slider.h
//
//
//

#pragma once
#include "Layer.h"
#include <functional>
class Slider: public Layer {
public:
	Layer *handle;
	Layer *rail;
	
	float *value;
	
	constexpr static float DEFAULT_HEIGHT = 30;
	constexpr static float HANDLE_WIDTH = 15;
	constexpr static float RADIUS = 4;
	
	float min = 0;
	float max = 1;
	
	Slider &darkTheme();
	
	void setValue(float f);
	
	Slider(Graphics &g, float *value = NULL, float min = 0, float max = 1);
	Slider(Graphics &g, float &val, float min = 0, float max = 1) : Slider(g, &val, min, max) {}
	
	void _resized();
	
	glm::vec2 originalDims;
	
	std::function<void(float)> valueChanged;
	
	void update() override;
	
	void drag(float x);
	
	bool touchDown(float x, float y, int id) override;
	
	void touchMoved(float x, float y, int id) override;

};


