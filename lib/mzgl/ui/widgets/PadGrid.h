

#pragma once

#include "Layer.h"

class PadGrid: public Layer {

public:
	PadGrid(Graphics &g, std::string name, float x, float y, float w, float h, float padding = 5);
	void setNumPads(int cols, int rows);
	void update() override;
	std::function<void(int)> padDown;
	std::function<void(int)> padUp;
	void setPadColor(int pad, glm::vec4 color);
    float padding = 5;
private:
	glm::vec2 originalDims;
	void resized();
	int rows;
	int cols;
	int indexToPadNo(int i);
};
