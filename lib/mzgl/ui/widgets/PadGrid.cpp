//
//  PadGrid.cpp
//  TapeSampler
//
//  Created by Marek Bereza on 14/11/2017.
//
//

#include "PadGrid.h"
#include "Graphics.h"
using namespace std;
class PadButton: public Layer {
public:
	vec4 color;
	PadButton(Graphics &g, int pos, glm::vec4 upColor = {0.5, 0.5, 0.5, 1.0}, glm::vec4 downColor = {0.8, 0.8, 0.8, 1.0}) : Layer(g, ""), downColor(downColor) {
		interactive = true;
		down = false;
		color = upColor;
	}
	
	
	void draw() override {
		
		if(down) {
			g.setColor(downColor);
		} else {
			g.setColor(color);
		}
		//ofDrawRectRounded(*this, 5);
		roundedRect.draw(g, *this, 5);
	}
	
	glm::vec4 downColor;
	std::function<void()> buttonPressed;
	std::function<void()> buttonReleased;
	
	virtual void touchUp(float x, float y, int id) override {
		if(inside(x, y) && interactive) {
			//ofSendMessage(name);
			if(buttonReleased) {
				buttonReleased();
			}
		}
		down = false;
	}
	void touchMoved(float x, float y, int id) override {
	}
	bool touchDown(float x, float y, int id) override {
		down = true;
		if(buttonPressed && interactive) {
			buttonPressed();
		}
		return true;
	}
private:
	bool down = false;
};






PadGrid::PadGrid(Graphics &g, string name, float x, float y, float w, float h, float padding) : Layer(g, name, x, y, w, h) {
	interactive = true;
	originalDims = glm::vec2(w, h);
	color = {0,0,0,0};
    this->padding = padding;
}

int PadGrid::indexToPadNo(int i) {
	int y = i / cols;
	int x = i % cols;
	return x + (rows - 1 - y) * cols;
}

void PadGrid::setNumPads(int cols, int rows) {
	clear ();
	
	this->rows = rows;
	this->cols = cols;
	
	int i = 0;
	for(int y = 0; y < rows; y++) {
		for(int x = 0; x < cols; x++) {
		
			
			auto b = new PadButton(g, i);
			addChild(b);
			b->buttonPressed = [i, this]() { if(padDown) padDown(i); };
			b->buttonReleased = [i, this]() { if(padUp) padUp(i); };
		
			
			++i;
		}
	}
	resized();
}

void PadGrid::resized() {
	
	
	
	float w = (width - padding * (cols - 1)) / (float)cols;
	float h = (height - padding * (rows - 1)) / (float)rows;

	int i = 0;
	for(int y = 0; y < rows; y++) {
		for(int x = 0; x < cols; x++) {

			Layer *b = getChild(indexToPadNo(i));
			b->set(x * (w + padding), y * (h + padding), w, h);
			
			b->x = (int) b->x; b->y = (int) b->y;
			b->width = (int) b->width; b->height = (int) b->height;

			++i;
		}
	}
	originalDims = glm::vec2(width, height);
}


void PadGrid::update() {
	if(width!=originalDims.x || height!=originalDims.y) {
		resized();
	}
}
void PadGrid::setPadColor(int pad, glm::vec4 color) {
    
}

