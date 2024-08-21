//
//  Knob.h
//  mzgl
//
//  Created by Marek Bereza on 11/06/2020.
//  Copyright © 2020 Marek Bereza. All rights reserved.
//

#pragma once

#include "Layer.h"
#include <string>

class Knob : public Layer {
public:
	std::string name;
	float &value;
	float min;
	float max;
	bool enabled = true;
	bool down	 = false;

	std::string niceToString(float val) {
		if (val < 2) {
			return to_string(val, 2);
		} else if (val < 50) {
			return to_string(val, 1);
		} else {
			return std::to_string((int) val);
		}
	}

	Knob(Graphics &g, std::string name, float &value, float min = 0, float max = 1)
		: Layer(g)
		, name(name)
		, value(value)
		, min(min)
		, max(max) {
		interactive = true;
		width		= 100;
		height		= 100;
	}

	void draw() override {
		vec2 c = centre();
		g.setColor(1);
		if (!enabled) g.noFill();
		else g.fill();

		g.drawCircle(c, width / 2);

		float angle = mapf(value, min, max, -225, 45);

		float theta = angle * M_PI / 180.f;

		g.pushMatrix();
		g.translate(c.x, c.y);
		g.rotateZ(theta);

		//		vec2 ang(cos(theta), sin(theta));
		//		vec2 a = c + ang * width * 0.5f;
		//		vec2 b = c + ang * 0.7f * width * 0.5f;
		g.setColor(0);
		//		g.drawLine(a, b);
		g.drawRect(width / 4, -2, width / 4, 4);

		g.popMatrix();
		g.setColor(1);
		g.drawTextCentred(name, {x + width / 2, y + height + 20});
		if (down) {
			g.setColor(0);
			g.drawTextCentred(niceToString(value), c);
		}
	}

	bool rounding = false;
	void setRounding(bool rounding) { this->rounding = rounding; }

	bool touchDown(float x, float y, int id) override {
		if (!enabled) return false;
		down = true;

		startValue	= value;
		startTouchY = y;
		currYTouch	= y;

		return true;
	}
	float currYTouch  = 0;
	float startTouchY = 0;
	float startValue  = 0;

	void touchMoved(float x, float y, int id) override {
		if (!enabled) return;

		currYTouch = y;
		float gain = 0.0025;
		value	   = startValue - (y - startTouchY) * gain * (max - min);

		value = std::clamp(value, min, max);
		if (rounding) value = round(value);
		if (value == -0.f) value = 0.f;
	}
	void touchUp(float x, float y, int id) override { down = false; }
};
