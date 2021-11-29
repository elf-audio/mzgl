//
//  TextButton.h
//
//

#pragma once
//
//  Button.h
//
//
//

#pragma once
#include <functional>
#include "Layer.h"
#include "ofMain.h"
#include "drawing.h"
class TextButton: public Layer {
public:
	bool down = false;
	TextButton(string name, function<void()> buttonReleased = nullptr) :
	Layer(name), buttonReleased(buttonReleased) {
		
		down = false;
		width = 200;
		height = 30;
		interactive = true;
	}
	
	void draw() {
		
		
		
		
		if(down) {
			ofSetColor(0);
			roundedRect.draw(*this, 2);
			ofSetColor(255);
			ofDrawBitmapString(name, getBottomLeft() + glm::vec2(10, -10));
		} else {
			ofSetColor(100);
			roundedRect.draw(*this, 2);
			ofSetColor(255);
			ofDrawBitmapString(name, getBottomLeft() + glm::vec2(10, -10));
		}
	}
	
	std::function<void()> buttonPressed;
	std::function<void()> buttonReleased;
	
	
	virtual void touchUp(float x, float y, int id) {
		//			auto func = [] () { cout << "Hello world"; };
		if(inside(x, y)) {
			ofSendMessage(name);
			if(buttonReleased) {
				buttonReleased();
			}

		}
		down = false;
	}
	virtual void touchMoved(float x, float y, int id) {
	}
	virtual bool touchDown(float x, float y, int id) {
		down = true;
		if(buttonPressed) {
			buttonPressed();
		}
		return true;
	}
	
};
