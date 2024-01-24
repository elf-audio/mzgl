//
//  Draggable.h
//
//
//

#pragma once

#include <mzgl/ui/Layer.h>

class Draggable : public Layer {
public:
	glm::vec2 lastTouch;
	bool vertical;
	bool horizontal;
	glm::vec2 currSpeed;
	bool down;
	float minX, minY, maxX, maxY;
	Draggable(string image, bool vertical = true, bool horizontal = true)
		: Layer(image, image) {
		this->vertical	  = vertical;
		this->horizontal  = horizontal;
		minX			  = -1;
		maxX			  = -1;
		minY			  = -1;
		maxY			  = -1;
		this->interactive = true;
		down			  = false;
	}

	void update() {
		if (!down) {
			glm::vec2 prevPos(this->x, this->y);

			currSpeed *= 0.86;
			if (ABS(currSpeed.x) < 1 && ABS(currSpeed.y) < 1) currSpeed.set(0, 0);
			this->x += currSpeed.x;
			this->y += currSpeed.y;

			clampX();
			clampY();

			glm::vec2 pos(this->x, this->y);
			glm::vec2 finalDelta = pos - prevPos;

			if (dragged) {
				dragged(finalDelta, false);
			}
		}
	}

	void setVerticalLimits(float minY, float maxY) {
		this->minY = MIN(minY, maxY);
		this->maxY = MAX(minY, maxY);
	}

	void setHorizontalLimits(float minX, float maxX) {
		this->minX = MIN(minX, maxX);
		this->maxX = MAX(minX, maxX);
	}

	virtual bool touchDown(float x, float y, int id) {
		lastTouch.set(x, y);
		down = true;
		return true;
	}
	void clampX() {
		if (minX != -1 && maxX != -1) {
			this->x = ofClamp(this->x, minX, maxX);
		}
	}

	void clampY() {
		if (minY != -1 && maxY != -1) {
			this->y = ofClamp(this->y, minY, maxY);
		}
	}
	virtual void touchMoved(float x, float y, int id) {
		glm::vec2 prevPos(this->x, this->y);
		glm::vec2 delta = glm::vec2(x, y) - lastTouch;

		if (horizontal) {
			this->x += delta.x;

			clampX();
		}
		if (vertical) {
			this->y += delta.y;

			clampY();
		}

		glm::vec2 pos(this->x, this->y);
		glm::vec2 finalDelta = pos - prevPos;

		if (dragged) {
			dragged(finalDelta, true);
		}
		//printf("%f %f\n", finalDelta.x, finalDelta.y);
		currSpeed = currSpeed * 0.667 + finalDelta * 0.333;

		lastTouch.set(x, y);
	}

	// the vec2 is the delta of the drag, the bool is whether the touch was
	// down (if false, it signifies the movement was due to inertia
	function<void(glm::vec2, bool)> dragged;

	// this gets called when the draggable gets released, with its current
	// velocity
	function<void(glm::vec2)> released;

	virtual void touchUp(float x, float y, int id) {
		down = false;
		if (released) {
			released(currSpeed);
		}
	}
};
