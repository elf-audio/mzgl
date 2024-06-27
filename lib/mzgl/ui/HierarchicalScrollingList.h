//
//  HierarchicalcrollingList.h
//  mzgl
//
//  Created by Marek Bereza on 12/10/2021.
//  Copyright © 2021 Marek Bereza. All rights reserved.
//

#pragma once
#include <mzgl/ui/ScrollingList.h>
#include <mzgl/animation/animation.h>
#include <mzgl/util/log.h>
#include <mzgl/util/mzAssert.h>
#include <list>
class HierarchicalScrollingList : public ScrollingList {
public:
	HierarchicalScrollingList(Graphics &g, float defaultHeight)
		: ScrollingList(g, defaultHeight) {}

	std::vector<Layer *> layersToRemove;

	class ScrollInfo {
	public:
		float scrollOffset = 0;
		int selectedIndex  = -1;
	};

	std::list<ScrollInfo> scrollOffsets;
	std::function<void()> backPressed = []() {};

	virtual bool keyDown(int key) override {
		if (key == MZ_KEY_LEFT) {
			backPressed();
			return true;
		}
		return ScrollingList::keyDown(key);
	}

	void push(const std::vector<std::shared_ptr<ScrollingListItem>> &items) {
		if (isAnimating()) {
			Log::e() << "Error: can't call push whilst animating";
			mzAssert(false);
		}

		// remove items from content temporarily and put them directly into this layer
		for (int i = 0; i < content->getNumChildren(); i++) {
			Layer *child = content->getChild(i);
			layersToRemove.push_back(child);
		}
		for (auto *l: layersToRemove) {
			l->removeFromParent();
			l->y += content->y;
			addChild(l);
		}

		scrollOffsets.push_back({content->y, getSelectedIndex()});
		selectedIndex = -1;
		content->y	  = 0;

		setItems(items);
		isPushing	 = true;
		animationAmt = 0;
		content->x	 = width;
		content->sendToFront();
	}

	float animationAmt = 0;
	void pop(const std::vector<std::shared_ptr<ScrollingListItem>> &items) {
		if (isAnimating()) {
			Log::e() << "Error: can't call pop whilst animating";
			mzAssert(false);
		}

		// remove items from content temporarily and put them directly into this layer
		for (int i = 0; i < content->getNumChildren(); i++) {
			Layer *child = content->getChild(i);
			layersToRemove.push_back(child);
		}
		for (auto *l: layersToRemove) {
			l->removeFromParent();
			l->y += content->y;
			addChild(l);
		}

		content->x	 = width * 0.25;
		animationAmt = 0;
		isPopping	 = true;

		setItems(items);

		if (!scrollOffsets.empty()) {
			content->y = scrollOffsets.back().scrollOffset;
			focus(scrollOffsets.back().selectedIndex);

			scrollOffsets.pop_back();
		} else {
			Log::e() << "Something went wrong, maybe popped more than pushed?";
		}
	}

	bool isAnimating() { return isPushing || isPopping; }

	bool isPushing	   = false;
	bool isPopping	   = false;
	VboRef toRemoveVbo = nullptr;

	void _draw() override {
		canSelect = !isAnimating();
		// you only want to draw the temp layers over the content if you're popping
		if (isPopping) {
			ScrollingList::_draw();
			float xx = easeOutCubic(animationAmt) * width;
			g.setColor(bgColor);
			g.drawRect(xx, y, width, height);
		}
		drawTempLayers();
		if (!isPopping) ScrollingList::_draw();
	}

	void drawTempLayers() {
		maskOn();
		if (layersToRemove.size() > 0) {
			Drawer d;
			for (int i = 0; i < layersToRemove.size(); i++) {
				// if on-screen...
				if (true) {
					((ScrollingListItemView *) layersToRemove[i])->draw(d);
				}
			}
			if (toRemoveVbo == nullptr) toRemoveVbo = Vbo::create();
			d.commit(toRemoveVbo);
			{
				// animate scrolling back out
				float xx = 0;
				if (isPushing) xx = x - easeOutCubic(animationAmt) * width * 0.25f;
				else if (isPopping) xx = easeOutCubic(animationAmt) * width;
				ScopedTranslate scpT(g, xx, y);
				g.setColor(1);
				toRemoveVbo->draw(g);

				for (int i = 0; i < layersToRemove.size(); i++) {
					// if on-screen...
					if (true) {
						layersToRemove[i]->_draw();
					}
				}
			}
		}
		maskOff();
	}

	void update() override {
		ScrollingList::update();
		if (isPushing || isPopping) {
			animationAmt += 0.05;

			if (isPushing) {
				content->x = (1 - easeOutCubic(animationAmt)) * width;
			} else if (isPopping) {
				content->x = -(1 - easeOutCubic(animationAmt)) * width * 0.25f;
			}
			if (animationAmt >= 1) {
				isPushing = false;
				isPopping = false;
				// switcheroo
				content->x = 0;
				for (auto *l: layersToRemove) {
					l->removeFromParent();
					delete l;
				}
				layersToRemove.clear();
			}
		}
	}
};
