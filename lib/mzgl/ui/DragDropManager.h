//
//  DragDropManager.h
//  mzgl
//
//  Created by Marek Bereza on 17/06/2022.
//  Copyright © 2022 Marek Bereza. All rights reserved.
//

#pragma once

#include "DrawingFunction.h"


// this is what your dragger should inherit from
class Dragger {
public:
	
	virtual ~Dragger() {}
	Layer *sourceLayer;
	Graphics &g;
	int touchId;
	
	/**
	 * @param sourceLayer the layer that received the touch (because we tranfer the touch away)
	 * @param startTouch  must be in absolute coordinates
	 * @param touchId the touch id of the touch dragging
	 */
	Dragger(Graphics &g, Layer *sourceLayer, glm::vec2 startTouch, int touchId) :
	g(g), startTouch(startTouch), sourceLayer(sourceLayer), touchId(touchId) {
		touchOffset = startTouch - sourceLayer->getAbsolutePosition();
	}
	
	glm::vec2 touchDelta;
	glm::vec2 startTouch;
	
	// distance from finger to top-left of dragging object
	vec2 touchOffset;
	
	
	// denotes if the user has moved their finger far enough
	// to initiate the drag due to hysteresisDistance
	bool isActive() { return active; }
	
	void activate() {
		active = true;
	}
	
	virtual void draw() {}
	
	// reposition the drag origin
	void setCentre(vec2 c) {
		startTouch = c;
		touchDelta = {0.f, 0.f};
		touchOffset = {0.f, 0.f};
	}
	
	vec2 touchPos() const {
		return startTouch + touchDelta;
	}
	
	void touchMoved(float x, float y) {
		auto touch = glm::vec2(x, y);
		touchDelta = touch - startTouch;
		
		if(!active) {
			if(glm::length(touchDelta)>hysteresisDistance) {//} * g.width / 750.f) {
				active = true;
			}
		}
	}
	
	void setHysteresisDistance(float f) {
		hysteresisDistance = f;
	}
	
private:
	float hysteresisDistance = 0;
	bool active = false;
};


template <class T>
class DropTarget : public Layer {
public:
	DropTarget(Graphics &g) : Layer(g) {}
	
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
};

template <class T>
class DragDropManager : public Layer {
public:
	
	/**
	 * The drag root is the root layer that all dragging happens from -
	 * things can be nested but we need to know which is the bottom layer.
	 */
	DragDropManager(Layer *dragRoot) : Layer(dragRoot->getGraphics()), dragRoot(dragRoot) {}
	
	// add all your targets ahead of time
	void addTarget(DropTarget<T> *target) {
		dropTargets.push_back(target);
	}
	
	
	bool dragsStartedCalled = false;
	
	// add draggers as items are dragged
	void addDragger(std::shared_ptr<T> dragger) {
		dragger->sourceLayer->transferFocus(this);
		draggers[dragger->touchId] = dragger;
	}
	
	void cancelAll() {
		callDragsEnded();
		for(auto &d : draggers) {
			auto id = d.first;
			auto dragger = d.second;
			// got to fire the touchUp event in order for the
			// original object to know we released it
			auto c = dragger->sourceLayer->centre();
			dragger->sourceLayer->touchUp(c.x, c.y, id);
		}
		draggers.clear();
	}
	

public:
	
	
	DrawingFunction *createDraggerDrawingFunction() {
		return new DrawingFunction(g, [this](Graphics &g) {
			drawDraggers();
		});
	}
	
	
	// call this explicitly - could make DragDropManager a layer
	// so it can just be added
	void drawDraggers() {
		if(!dragsStartedCalled) {
			for(auto &d : draggers) {
				if(d.second->isActive()) {
					callDragsStarted();
					break;
				}
			}
		}
		for(auto &d : draggers) {
			d.second->draw();
		}
	}

	bool hasTouch(int touchId) {
		return draggers.find(touchId)!=draggers.end();
	}
	
	void touchMoved(float x, float y, int id) override {
		touchMoved__(x, y, id);
	}
	
	void touchUp(float x, float y, int id) override {
		touchUp__(x, y, id);
	}
	
	bool touchMoved__(float x, float y, int id) {
		if(draggers.find(id)!=draggers.end()) {
			
			auto d = draggers[id];
			auto prevPos = d->touchPos();
			
			d->touchMoved(x, y);
			
			if(d->isActive()) {
				
				auto currPos = d->touchPos();
				
				for(auto *target : dropTargets) {
					
					Rectf r;
					
					if(target->getRectRelativeTo(dragRoot, r)) {
						
						bool wasInside = r.inside(prevPos);
						bool isInside = r.inside(currPos);

						if(isInside && !wasInside) target->draggedIn(d);
						else if(!isInside && wasInside) target->draggedOut(d);
					}
				}
			}
			return true;
		}
		return false;
	}
	
	bool touchUp__(float x, float y, int id) {
		if(draggers.find(id)!=draggers.end()) {
			
			// make sure we've reached the drag threshold
			if(draggers[id]->isActive()) {
				auto touch = draggers[id]->touchPos();
				for(auto *t : dropTargets) {
					Rectf r;
					if(t->getRectRelativeTo(dragRoot, r)) {
						if(r.inside(touch)) {
							t->dropped(draggers[id]);
						}
					}
				}
			}
			// got to fire the touchUp event in order for the
			// original object to know we released it
			draggers[id]->sourceLayer->touchUp(x, y, id);
			draggers.erase(id);
			if(draggers.empty()) {
				callDragsEnded();
			}
			return true;
		}
		return false;
	}

	
	std::map<int,std::shared_ptr<T>> draggers;
	
private:

	void callDragsStarted() {
		dragsStartedCalled = true;
		for(auto *d : dropTargets) {
			d->dragsStarted();
		}
	}
	
	void callDragsEnded() {
		dragsStartedCalled = false;
		for(auto *d : dropTargets) {
			d->dragsEnded();
		}
	}
	
	Layer *dragRoot = nullptr;
	std::vector<DropTarget<T>*> dropTargets;
};


