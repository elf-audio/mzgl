//
//  Button.h
//
//
//

#pragma once
#include <functional>
#include <mzgl/ui/Layer.h>

class SimpleButton : public Layer {
public:
	Texture upImg;
	Texture downImg;
	bool down = false;
	SimpleButton(string name, string imagePath = "", string downImagePath = "")
		: Layer(name) {
		if (imagePath != "") {
			upImg.load(imagePath);
		}

		if (downImagePath != "") {
			downImg.load(downImagePath);
		}

		down		= false;
		width		= upImg.width;
		height		= height;
		interactive = true;
	}

	virtual void draw() {
		g.setColor(color);
		if (down) {
			if (downImg.width > 0) {
				downImg.draw(g, *this);
			} else {
				// if there's no down image, draw the up image
				// with 50% alpha.
				glm::vec4 c = color;
				c.a *= 0.5;
				g.setColor(c);
				upImg.draw(g, *this);
			}
		} else {
			upImg.draw(g, *this);
		}
	}

	std::function<void()> buttonPressed;
	std::function<void()> buttonReleased;

	virtual void touchUp(float x, float y, int id) {
		if (inside(x, y) && interactive) {
			if (buttonReleased) {
				buttonReleased();
			}
		}
		down = false;
	}
	virtual void touchMoved(float x, float y, int id) {}
	virtual bool touchDown(float x, float y, int id) {
		down = true;
		if (buttonPressed && interactive) {
			buttonPressed();
		}
		return true;
	}
};
