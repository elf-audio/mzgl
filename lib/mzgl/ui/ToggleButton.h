//
//  Button.h
//
//
//

#pragma once
#include <functional>
#include "Layer.h"
#include "ofMain.h"

class ToggleButton : public Layer {
public:
	ofImage upImg;
	ofImage downImg;
	ofImage onUpImg;
	ofImage onDownImg;

	bool down;
	bool value;

	ToggleButton(string name, string imagePath, string downImagePath, string upOnImagePath, string downOnImagePath)
		: Layer(name) {
		upImg.load(imagePath);
		downImg.load(downImagePath);
		onUpImg.load(upOnImagePath);
		onDownImg.load(downOnImagePath);

		value		= false;
		down		= false;
		width		= upImg.getWidth();
		height		= upImg.getHeight();
		interactive = true;
	}

	void draw() {
		ofSetColor(color);
		if (down) {
			if (value) {
				onDownImg.draw(*this);
			} else {
				downImg.draw(*this);
			}
		} else {
			if (value) {
				onUpImg.draw(*this);
			} else {
				upImg.draw(*this);
			}
		}
	}

	std::function<void()> buttonPressed;
	std::function<void()> buttonReleased;
	std::function<void(bool)> toggled;

	virtual void touchUp(float x, float y, int id) {
		//			auto func = [] () { cout << "Hello world"; };

		if (inside(x, y)) {
			ofSendMessage(name);
			if (buttonReleased) {
				buttonReleased();
			}
			value ^= true;
			if (toggled) {
				toggled(value);
			}
		}

		down = false;
	}
	virtual void touchMoved(float x, float y, int id) {}
	virtual bool touchDown(float x, float y, int id) {
		down = true;
		if (buttonPressed) {
			buttonPressed();
		}
		return true;
	}
};
;
