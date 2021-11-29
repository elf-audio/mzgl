#pragma once
#include "Layer.h"
#include "Tween.h"
/**
 * UI Widgets that look sort of like the ios ones
 */
class NiceSlider: public Layer {
public:
	float min = 0;
	float max = 1;
	float *value = NULL;
	
	NiceSlider(float *value, float min = 0, float max = 1)
	: Layer(""), min(min), max(max), value(value) {
		
		width = 200;
		height = 30;
		interactive = true;
	}
	RoundedRect railBg;
	RoundedRect highlight;
	void draw() override {
		float radius = height / 2;
		float railThickness = 8;
		float xx = mapf(*value, min, max, x + radius, x + width - radius);
		
		setColor(hexColor(0x474747));
		// rail bg
		railBg.draw(Rectf(x + radius, y + radius - railThickness/2, width - radius * 2), railThickness);
		
		// highlight
		setColor(hexColor(0x900000));
		highlight.draw(Rectf(x + radius, y + radius - railThickness/2,
						mapf(*value, min, max, 0, width - radius * 2))
						, railThickness);
		
		setColor(hexColor(0xb2b2b2));
		drawCircle(xx, y + radius, radius);
		
		
	}
	
	bool touchDown(float x, float y, int id) override {
		float radius = height / 2;
		*value = mapf(x, this->x + radius, this->x + width - radius, min, max, true);
		return true;
	}
	
	void touchMoved(float x, float y, int id) override {
		float radius = height / 2;
		*value = mapf(x, this->x + radius, this->x + width - radius, min, max, true);
	}
};

class NiceToggle: public Layer {
public:
	NiceToggle() : Layer("") {
		interactive = true;
		width = 110;
		height = 60;
	}
	RoundedRect outline;
	RoundedRect bg;
	RoundedRect handle;
	
	/*color.setHex(0x474747);
	 rail->color.setHex(0x232323);
	 handle->color.setHex(0xb2b2b2);*/
	void draw() override {
		fill();
		
		glm::vec4 bgC = hexColor(0x474747);
		glm::vec4 highlight = hexColor(0x900000);
		
		setColor(bgC * (1.f - amt) + highlight * amt);
		bg.draw(*this, height/2);
		Rectf r = *this;
		r.width = mapf(decidey, 0, 1, r.height, r.height + (r.width - r.height) * 0.3);
		r.x = mapf(amt, 0, 1, x, x + width - height) - (r.width - r.height)*amt;
		int p = 6;
		r.x += p;
		r.y += p;
		r.width -= p * 2;
		r.height -= p * 2;
		setColor(hexColor(0xb2b2b2));
		handle.draw(r, r.height/2);
		
		
		noFill();
		setColor(hexColor(0x232323));
		outline.draw(*this, height/2);
	}
	
	bool down = false;
	bool value = false;
	
	float amt = 0;
	float decidey = 0;
	
	bool touchDown(float x, float y, int id) override {
		down = true;
		EASE_OUT(decidey, 1, 0.25);
		return true;
	}
	
	void touchUp(float x, float y, int id) override {
		if(inside(x, y)) {
			value = !value;
			EASE_OUT(amt, value?1:0, 0.4);
			EASE_OUT(decidey, 0, 0.25);
		}
		down = false;
	}
};
