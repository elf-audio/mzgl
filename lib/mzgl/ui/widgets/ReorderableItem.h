//
//  ReorderableItem.h
//  mzgl
//
//  Created by Marek Bereza on 21/04/2022.
//  Copyright © 2022 Marek Bereza. All rights reserved.
//

#pragma once

#include "Layer.h"

class ReorderableItem : public Layer {
public:
	int slot;
	int index;
	ReorderableItem(Graphics &g, int slot)
		: Layer(g)
		, slot(slot) {
		targetRect = *this;
		index	   = slot;
	}

	virtual void draw(Graphics &g, bool isReordering) {}

	void startDragging(vec2 start, int touchId) {
		touchStart	  = start;
		this->touchId = touchId;
		dragging	  = true;
	}

	int update(const std::vector<Rectf> &slots, float x, float y) {
		vec2 touchDelta = vec2(x, y) - touchStart;
		this->x			= targetRect.x + touchDelta.x;
		this->y			= targetRect.y + touchDelta.y;
		for (int i = 0; i < slots.size(); i++) {
			if (i != slot && slots[i].inside(x, y)) {
				return i;
			}
		}
		return -1;
	}
	void updateDeprecated() override {
		if (!dragging && !atOriginalPosition()) {
			x = x * 0.8 + targetRect.x * 0.2;
			y = y * 0.8 + targetRect.y * 0.2;
		}
	}
	void stopDragging(const std::vector<Rectf> &slots) {
		dragging   = false;
		targetRect = slots[slot];
	}

	bool atOriginalPosition() { return abs(x - targetRect.x) < 0.5 && abs(y - targetRect.y) < 0.5; }

	void setTargetSlot(const std::vector<Rectf> &slots, int newSlot) {
		if (!dragging) {
			targetRect = slots[newSlot];
		}
		slot = newSlot;
	}

	void setPosition(const Rectf &r) {
		targetRect.set(r);
		this->set(r);
	}

	Rectf targetRect;
	bool dragging = false;
	int touchId	  = -1;
	vec2 touchStart;
};
