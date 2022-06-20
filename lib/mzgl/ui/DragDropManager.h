//
//  DragDropManager.h
//  mzgl
//
//  Created by Marek Bereza on 17/06/2022.
//  Copyright © 2022 Marek Bereza. All rights reserved.
//

#pragma once


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
class DragDropManager {
public:
	
	/**
	 * The drag root is the root layer that all dragging happens from -
	 * things can be nested but we need to know which is the bottom layer.
	 */
	DragDropManager(Layer *dragRoot) : dragRoot(dragRoot) {}
	
	// add all your targets ahead of time
	void addTarget(DropTarget<T> *target) {
		dropTargets.push_back(target);
	}
	
	// add draggers as items are dragged
	void addDragger(int touchId, std::shared_ptr<T> dragger) {
		bool firstDragger = draggers.empty();
		draggers[touchId] = dragger;
		if(firstDragger) {
			for(auto *d : dropTargets) {
				d->dragsStarted();
			}
		}
	}
	
	// call this explicitly - could make DragDropManager a layer
	// so it can just be added
	void drawDraggers() {
		for(auto &d : draggers) {
			d.second->draw();
		}
	}

	bool hasTouch(int touchId) {
		return draggers.find(touchId)!=draggers.end();
	}
	
	bool touchMoved(float x, float y, int id) {
		if(draggers.find(id)!=draggers.end()) {
			
			auto d = draggers[id];
			auto prevPos = d->touchPos();
			
			d->touchMoved(x, y);
			
			if(d->dragging) {
				
				auto currPos = d->touchPos();
				
				for(auto *target : dropTargets) {
					
					Rectf r;
					
					if(target->getRectRelativeTo(dragRoot, r)) {
						
						bool wasInside = r.inside(prevPos);
						bool isInside = r.inside(currPos);
//						printf("%sside %s\n", isInside?"in":"out", target->name.c_str());
						if(isInside && !wasInside) target->draggedIn(d);
						else if(!isInside && wasInside) target->draggedOut(d);
	
						
					}
					
				}
			}
			return true;
		}
		return false;
	}
	
	bool touchUp(float x, float y, int id) {
		if(draggers.find(id)!=draggers.end()) {
			
			// make sure we've reached the drag threshold
			if(draggers[id]->dragging) {
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
			draggers.erase(id);
			if(draggers.empty()) {
				for(auto *t : dropTargets) {
					t->dragsEnded();
				}
			}
			return true;
		}
		return false;
	}
	
	std::map<int,std::shared_ptr<T>> draggers;
	
private:
	Layer *dragRoot = nullptr;
	std::vector<DropTarget<T>*> dropTargets;
	
};


