//
//  DragDropManager.h
//  mzgl
//
//  Created by Marek Bereza on 17/06/2022.
//  Copyright © 2022 Marek Bereza. All rights reserved.
//

#pragma once

#include "DrawingFunction.h"
#include "mzAssert.h"
#include <memory>

// this is what your dragger should inherit from
class Dragger {
public:
	virtual ~Dragger() {}

	Rectf sourceLayerRect;
	Graphics &g;
	// the area on the screen that the drag has been dragged
	Rectf dragRect;
	int touchId;

	/**
	 * @param sourceLayer the layer that received the touch (because we tranfer the touch away)
	 * @param startTouch  must be in absolute coordinates
	 * @param touchId the touch id of the touch dragging
	 */
	Dragger(Graphics &g, Layer *sourceLayer, glm::vec2 startTouch, int touchId)
		: g(g)
		, startTouch(startTouch)
		, sourceLayer(sourceLayer)
		, sourceLayerRect(sourceLayer->getAbsoluteRect())
		, touchId(touchId)
		, creationTime(g.currFrameTime) {
		touchOffset = startTouch - sourceLayer->getAbsolutePosition();
		dragRect.set(startTouch.x, startTouch.y, 0, 0);
		touchDelta = {0.f, 0.f};
	}

	const double creationTime;
	glm::vec2 touchDelta;
	glm::vec2 startTouch;

	// distance from finger to top-left of dragging object
	vec2 touchOffset;

	// denotes if the user has moved their finger far enough
	// to initiate the drag due to hysteresisDistance
	bool isActive() { return active; }

	void activate() {
		active = true;
		activated();
	}

	// gets called when dragger appears (has moved past hysteresis distance)
	std::function<void()> activated = []() {};

	// call this if you don't want the
	// sourceLayer to receive a touch up
	// when you drop the drag.
	void cancelTouchUpOnSourceLayer() { sourceLayer = nullptr; }

	virtual void draw() {}

	// reposition the drag origin
	void setCentre(vec2 c) {
		startTouch	= c;
		touchDelta	= {0.f, 0.f};
		touchOffset = {0.f, 0.f};
	}

	vec2 touchPos() const { return startTouch + touchDelta; }

	void touchMoved(float x, float y) {
		auto touch = glm::vec2(x, y);
		dragRect.growToInclude(touch);
		touchDelta = touch - startTouch;

		if (!active) {
			if (glm::length(touchDelta) > hysteresisDistance) {
				activate();
			}
		}
	}

	void setHysteresisDistance(float f) { hysteresisDistance = f; }

	// may be null, so always check
	Layer *sourceLayer = nullptr;

private:
	float hysteresisDistance = 0;
	bool active				 = false;
};

template <class T>
class DropTarget : public Layer {
public:
	DropTarget(Graphics &g)
		: Layer(g)
		, lifeToken(std::make_shared<char>()) {}

	// called when a T is dragged into this drop target
	virtual void draggedIn(std::shared_ptr<T> dragger) {}
	// called when a T is dragged out of this drop target
	virtual void draggedOut(std::shared_ptr<T> dragger) {}

	// called when a T is dropped onto this drop target
	virtual void dropped(std::shared_ptr<T> dragger) {}

	// gets called when any drag is started when there were
	// no drags before
	virtual void dragsStarted() {}

	// gets called when all drags are finished
	virtual void dragsEnded() {}

	// when this DropTarget is destroyed, lifeToken dies and
	// DragDropManager's weak_ptr to it expires automatically.
	// This is to fix a bug on android that I haven't been able
	// to crack - I put this in on 12/03/26 so if the bug is
	// still there on subsequent versions, pls remove lifeToken
	// related code below.
	std::shared_ptr<void> lifeToken;
};

template <class T>
class DragDropManager : public Layer {
public:
	/**
	 * The drag root is the root layer that all dragging happens from -
	 * things can be nested but we need to know which is the bottom layer.
	 */
	DragDropManager(Layer *dragRoot)
		: Layer(dragRoot->getGraphics())
		, dragRoot(dragRoot) {}

	// add all your targets ahead of time
	void addTarget(DropTarget<T> *target) {
		purgeDeadTargets();
		dropTargets.push_back({target, target->lifeToken});
	}
	void removeTarget(DropTarget<T> *target) {
		for (auto it = dropTargets.begin(); it != dropTargets.end(); it++) {
			if (it->target == target) {
				dropTargets.erase(it);
				break;
			}
		}
	}

	// add draggers as items are dragged
	void addDragger(std::shared_ptr<T> dragger) {
		dragger->sourceLayer->transferFocus(this, dragger->touchId);
		draggers[dragger->touchId] = dragger;
	}

	void cancelAll() {
		callDragsEnded();
		for (auto &d: draggers) {
			auto id		 = d.first;
			auto dragger = d.second;
			// got to fire the touchUp event in order for the
			// original object to know we released it
			if (dragger->sourceLayer != nullptr) {
				auto c = dragger->sourceLayer->centre();
				dragger->sourceLayer->touchUp(c.x, c.y, id);
			}
		}
		draggers.clear();
	}

	void cancel(std::shared_ptr<T> dragger, bool passTouchBackToSourceLayer) {
		for (auto it = draggers.begin(); it != draggers.end(); it++) {
			if ((*it).second == dragger) {
				if (passTouchBackToSourceLayer) {
					transferFocus(dragger->sourceLayer, dragger->touchId);
				} else {
					if (dragger->sourceLayer != nullptr) {
						auto c = dragger->sourceLayer->centre();
						dragger->sourceLayer->touchUp(c.x, c.y, dragger->touchId);
					}
				}
				draggers.erase(it);
				break;
			}
		}

		if (draggers.size() == 0) {
			callDragsEnded();
		}
	}

	/**
	 * DragDropManager assumes that all sourceLayers (e.g. the layer that originated
	 * the drag) will still be alive when drag is finished - if they are deleted
	 * sourceLayer in Dragger will become a dangling pointer, so we need to nullify it
	 * if the actual sourceLayer gets deleted. Bit of a hack sorry.
	 *
	 * It happens when you start a drag with the mediabrowser open, but then close the
	 * media browser or do anyhting to cause doLayout to recreate the layers.
	 */
	void removeAllSourceLayers() {
		for (auto &d: draggers) {
			d.second->sourceLayer = nullptr;
		}
	}

public:
	bool isActive() { return draggers.size() > 0; }

	DrawingFunction *createDraggerDrawingFunction() {
		return new DrawingFunction(g, [this](Graphics &g) { drawDraggers(); });
	}

	// call this explicitly - could make DragDropManager a layer
	// so it can just be added
	void drawDraggers() {
		if (!dragsStartedCalled) {
			for (auto &d: draggers) {
				if (d.second->isActive()) {
					callDragsStarted();
					break;
				}
			}
		}
		for (auto &d: draggers) {
			d.second->draw();
		}
	}

	bool hasTouch(int touchId) { return draggers.find(touchId) != draggers.end(); }

	void touchMoved(float x, float y, int id) override { touchMoved__(x, y, id); }

	void touchUp(float x, float y, int id) override { touchUp__(x, y, id); }

	bool touchMoved__(float x, float y, int id) {
		if (draggers.find(id) != draggers.end()) {
			auto d		 = draggers[id];
			auto prevPos = d->touchPos();

			d->touchMoved(x, y);

			if (d->isActive()) {
				auto currPos = d->touchPos();

				for (auto &entry: dropTargets) {
					if (entry.life.expired()) {
						mzAssert(false, "DragDropManager: drop target was destroyed without being removed");
						continue;
					}
					Rectf r;

					if (entry.target->getRectRelativeTo(dragRoot, r)) {
						bool wasInside = r.inside(prevPos);
						bool isInside  = r.inside(currPos);

						if (isInside && !wasInside) entry.target->draggedIn(d);
						else if (!isInside && wasInside) entry.target->draggedOut(d);
					}
				}
			}
			return true;
		}
		return false;
	}

	bool touchUp__(float x, float y, int id) {
		if (draggers.find(id) != draggers.end()) {
			// make sure we've reached the drag threshold
			if (draggers[id]->isActive()) {
				auto touch = draggers[id]->touchPos();
				for (auto &entry: dropTargets) {
					if (entry.life.expired()) {
						mzAssert(false, "DragDropManager: drop target was destroyed without being removed");
						continue;
					}
					Rectf r;
					if (entry.target->getRectRelativeTo(dragRoot, r)) {
						if (r.inside(touch)) {
							entry.target->dropped(draggers[id]);
						}
					}
				}
			}
			// got to fire the touchUp event in order for the
			// original object to know we released it
			if (draggers[id]->sourceLayer != nullptr) {
				// this was doing the incorrect coords as of 19.03.23,
				// fixed it to do local coords - it may have an impact elsewhere
				auto localCoords = draggers[id]->sourceLayer->getLocalPosition({x, y});
				draggers[id]->sourceLayer->touchUp(localCoords.x, localCoords.y, id);
			}

			draggers.erase(id);
			if (draggers.empty()) {
				callDragsEnded();
			}
			return true;
		}
		return false;
	}

	std::map<int, std::shared_ptr<T>> draggers;

private:
	bool dragsStartedCalled = false;

	void purgeDeadTargets() {
		dropTargets.erase(std::remove_if(dropTargets.begin(),
										 dropTargets.end(),
										 [](const TargetEntry &e) { return e.life.expired(); }),
						  dropTargets.end());
	}

	void callDragsStarted() {
		dragsStartedCalled = true;
		for (auto &entry: dropTargets) {
			if (entry.life.expired()) {
				mzAssert(false, "DragDropManager: drop target was destroyed without being removed");
				continue;
			}
			entry.target->dragsStarted();
		}
	}

	void callDragsEnded() {
		dragsStartedCalled = false;
		for (auto &entry: dropTargets) {
			if (entry.life.expired()) {
				mzAssert(false, "DragDropManager: drop target was destroyed without being removed");
				continue;
			}
			entry.target->dragsEnded();
		}
	}

	struct TargetEntry {
		DropTarget<T> *target;
		std::weak_ptr<void> life;
	};

	Layer *dragRoot = nullptr;
	std::vector<TargetEntry> dropTargets;
};
