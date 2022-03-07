//
//  DrawingFunction.h
//
//  This is a nice kind of layer, that you can just pass in a draw function
//  rather than creating a whole class.
//

#pragma once

#include "Layer.h"

class DrawingFunction: public Layer {
public:
	std::function<void(Graphics &g)> func;
	DrawingFunction(Graphics &g, std::function<void(Graphics &g)> func): Layer(g), func(func) {}
	
	void draw() override {
		func(g);
	}
};
