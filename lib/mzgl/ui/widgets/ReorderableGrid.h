//
//  RearrangableGrid.h
//  mzgl
//
//  Created by Marek Bereza on 24/06/2020.
//  Copyright © 2020 Marek Bereza. All rights reserved.
//

#pragma once


class ReorderableItem : public Layer {
public:
	int slot;
	int index;
	ReorderableItem(Graphics &g, int slot) : Layer(g), slot(slot) {
		//		set(slots[slot]);
		targetRect = *this;
		index = slot;
	}
	
	virtual void draw(Graphics &g, bool isReordering) {}

	
	void startDragging(vec2 start, int touchId) {
		touchStart = start;
		this->touchId = touchId;
		dragging = true;
	}
	
	int update(const vector<Rectf> &slots, float x, float y) {
		vec2 touchDelta = vec2(x, y) - touchStart;
		this->x = targetRect.x + touchDelta.x;
		this->y = targetRect.y + touchDelta.y;
		for(int i = 0; i < slots.size(); i++) {
			if(i!=slot && slots[i].inside(x, y)) {
				return i;
			}
		}
		return -1;
	}
	void update() override {
		if(!dragging && !atOriginalPosition()) {
			x = x * 0.8 + targetRect.x * 0.2;
			y = y * 0.8 + targetRect.y * 0.2;
		}
	}
	void stopDragging(const vector<Rectf> &slots) {
		dragging = false;
		targetRect = slots[slot];
	}
	
	bool atOriginalPosition() {
		return abs(x-targetRect.x)<0.5
		&&
		abs(y - targetRect.y)<0.5;
	}
	
	void setTargetSlot(const vector<Rectf> &slots, int newSlot) {
		if(!dragging) {
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
	int touchId = -1;
	vec2 touchStart;
};




class ReorderableGrid : public Layer {
public:
	vector<Rectf> slotPositions;
	vector<ReorderableItem*> items;
	function<void(const vector<int> &)> rearranged;
	
	// this is how much padding there is between
	// each slotPosition.
	int spacingAmt = 20;
	
	ReorderableGrid(Graphics &g) : Layer(g) {
		interactive = true;
		width = g.width;
		height = g.height;
	}
	
	void addItem(ReorderableItem *item) {
		items.push_back(item);
		addChild(item);
	}

	void setOrder(const vector<int> &order) {
		if(slotPositions.size() < order.size()) {
			Log::d() << "Error! trying to set positions before laying out!";
			return;
		}
		for(int i = 0; i < order.size(); i++) {
			items[order[i]]->set(slotPositions[i]);
			items[order[i]]->targetRect.set(slotPositions[i]);
			items[order[i]]->slot = i;
		}
	}
	bool isReordering() {
		return reordering;
	}
	void setReordering(bool reordering) {
		this->reordering = reordering;
		for(auto *c : items) {
			c->interactive = !reordering;
		}
	}
	void doLayout(int numCols) {
		
		slotPositions.clear();
		float itemWidth = (width - (numCols-1)*spacingAmt) / numCols;
		
		int numRows = items.size() / numCols;
		if(items.size() % numCols != 0) {
			numRows += 1;
		}
		float itemHeight = (height - (numRows-1)*spacingAmt) / numRows;
		for(int i = 0; i < items.size(); i++) {
			int xx = i % numCols;
			int yy = i / numCols;
			slotPositions.push_back({xx*(itemWidth+spacingAmt), yy*(itemHeight+spacingAmt), itemWidth, itemHeight});
		}
		
		for(int i = 0; i < slotPositions.size(); i++) {
			items[i]->setPosition(slotPositions[items[i]->slot]);
		}
	}
	
	// override this to do a custom slot position.
	virtual void drawSlotPosition(const Rectf &r) {
		
		if(reordering) {
			g.setColor(0, 0, 0, 0.1);
			g.fill();
			g.drawRoundedRect(r, 30);
		}
	}
	
	void draw() override {
		
		g.pushMatrix();
		g.translate(x, y);
		for(auto & sl : slotPositions) {
			drawSlotPosition(sl);
		}
		
		// draw all non-dragging items
		for(auto *item : items) {
			if(!item->dragging) {
				item->draw(g, reordering);
			}
		}
		
		// draw all dragging items
		for(auto *item : items) {
			if(item->dragging) {
				item->draw(g, reordering);
			}
		}
		g.popMatrix();
	}
	
	bool touchDown(float x, float y, int id) override {
		if(!reordering) return false;
		x -= this->x;
		y -= this->y;
		for(auto *item : items) {
			if(item->inside(x, y)) {
				item->startDragging({x, y}, id);
				return true;
			}
		}
		return false;
	}
	
	vector<int> getSlots() {
		vector<int> sl;
		for(int slot = 0; slot < items.size(); slot++) {
			for(int i = 0; i < items.size(); i++) {
				if(slot==items[i]->slot) {
					sl.push_back(items[i]->index);
					break;
				}
			}
		}
		return sl;
	}
	
	
	void touchMoved(float x, float y, int id) override {
		if(!reordering) return;
		x -= this->x;
		y -= this->y;
		// mapping from index to new slot pos
		map<int,int> moves;
		for(auto *item : items) {
			if(item->dragging && item->touchId==id) {
				int move = item->update(slotPositions, x, y);
				if(move!=-1) {
					moves[item->index] = move;
				}
			}
		}
		
		if(moves.size()>0) {
			vector<int> newSlots(items.size(), -1);
			for(auto &m : moves) {
				newSlots[m.second] = m.first;
			}
			vector<int> nonDraggingSlots = getSlots();
			for(int i = 0; i < nonDraggingSlots.size(); i++) {
				if(moves.find(nonDraggingSlots[i])!=moves.end()) {
					nonDraggingSlots.erase(nonDraggingSlots.begin() + i);
					i--;
				}
			}
			
			int next =  0;

			for(int i  = 0; i < newSlots.size(); i++) {
				if(newSlots[i]==-1) {
					newSlots[i] = nonDraggingSlots[next++];
				}
			}
			
			for(int i = 0; i < newSlots.size(); i++) {
				items[newSlots[i]]->setTargetSlot(slotPositions, i);
			}
			if(rearranged) rearranged(newSlots);
		}
	}
	
	void touchUp(float x, float y, int id) override {
		if(!reordering) return;
		x -= this->x;
		y -= this->y;
		for(auto *item : items) {
			if(item->dragging && item->touchId==id) {
				item->stopDragging(slotPositions);
			}
		}
	}
protected:
	bool reordering = false;

};


