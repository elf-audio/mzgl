//
//  Slider.cpp
//  TapeSampler
//
//  Created by Marek Bereza on 22/11/2017.
//
//


#include "Slider.h"
#include "maths.h"
Slider &Slider::darkTheme() {
	color = hexColor(0x474747);
	rail->color = hexColor(0x232323);
	handle->color = hexColor(0xb2b2b2);
	return *this;
}

void Slider::setValue(float f) {
	drag(mapf(f, min, max, x + handle->width / 2, x + width - handle->width / 2));
}

Slider::Slider(Graphics &g, float *value, float min, float max): Layer(g, "") {
	this->value = value;
	this->min = min;
	this->max = max;
	width = 200;
	height = DEFAULT_HEIGHT;
	
	//borderWidth = 1;
	//borderColor.setHex(0x000000);
	
	rail = new Layer(g, "");
	
	
	
	rail->color = glm::vec4(0.6, 0.6, 0.6, 1);
	addChild(rail);
	
	
	handle = new Layer(g, "");
	handle->width = HANDLE_WIDTH;
	
	_resized();
	handle->color = hexColor(0xCCCCCC);
	
//	handle->cornerRadius = RADIUS;//handle->width / 2;
//	cornerRadius = RADIUS;
	
	addChild(handle);
	interactive = true;
	originalDims = glm::vec2(width, height);
}



void Slider::_resized() {
	rail->x = HANDLE_WIDTH / 2 + RADIUS;
	rail->width = width - HANDLE_WIDTH - RADIUS * 2;
	rail->height = 2;
	
	rail->y = height / 2;
	
	if(value==NULL) {
		handle->x = RADIUS;
	} else {
		setValue(*value);
	}
	handle->y = RADIUS;
	
	handle->height = height - RADIUS * 2;
	originalDims = glm::vec2(width, height);
	
}

void Slider::update() {
	if(originalDims!=glm::vec2(width,height)) {
		_resized();
	}
}


void Slider::drag(float x) {
	handle->x = x - this->x - handle->width / 2;
	if(handle->centre().x < rail->x) handle->x = rail->x - handle->width / 2;
	if(handle->centre().x > rail->right()) handle->x = rail->right() - handle->width / 2;
	
	float val = mapf(handle->centre().x, rail->x, rail->right() - handle->width, min, max);
	
	if(value!=NULL) {
		*value = val;
	}
	
	if(valueChanged) {
		valueChanged(val);
	}
}

bool Slider::touchDown(float x, float y, int id) {
	
	
	if(inside(x, y)) {
		drag(x);
		
		return true;
	}
	return false;
	
}

void Slider::touchMoved(float x, float y, int id) {
	drag(x);
}

