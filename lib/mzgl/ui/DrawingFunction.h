//
//  DrawingFunction.h
//
//  This is a nice kind of layer, that you can just pass in a draw function and some dimensions
//  rather than creating a whole class.
//
//
//

#pragma once

#include "Layer.h"

class DrawingFunction: public Layer {
public:
	std::function<void(Graphics &g)> func;
	DrawingFunction(Graphics &g, std::function<void(Graphics &g)> func, float x = 0, float y = 0, float width = 0, float height = 0):
	Layer(g, "") {
		set(x, y, width, height);
		this->func = func;
	}
	
	void draw() {
		func(g);
	}
};
