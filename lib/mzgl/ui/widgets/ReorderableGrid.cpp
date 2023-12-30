//
//  RearrangableGrid.h
//  mzgl
//
//  Created by Marek Bereza on 24/06/2020.
//  Copyright © 2020 Marek Bereza. All rights reserved.
//

#include "ReorderableGrid.h"

#include "log.h"

ReorderableGrid::ReorderableGrid(Graphics &g)
	: Layer(g) {
	interactive = true;
	name		= "reorderable grid";
	width		= g.width;
	height		= g.height;
}

void ReorderableGrid::addItem(ReorderableItem *item) {
	items.push_back(item);
	addChild(item);
}

void ReorderableGrid::setOrder(const std::vector<int> &order) {
	if (slotPositions.size() < order.size()) {
		Log::d() << "Error! trying to set positions before laying out!";
		return;
	}
	for (int i = 0; i < order.size(); i++) {
		items[order[i]]->set(slotPositions[i]);
		items[order[i]]->targetRect.set(slotPositions[i]);
		items[order[i]]->slot = i;
	}
}

bool ReorderableGrid::isReordering() {
	return reordering;
}

void ReorderableGrid::setReordering(bool reordering) {
	this->reordering = reordering;
	for (auto *c: items) {
		c->interactive = !reordering;
	}
}

void ReorderableGrid::doLayoutWithPositions(const std::vector<Rectf> &newSlots) {
	slotPositions = newSlots;
	for (int i = 0; i < slotPositions.size(); i++) {
		items[i]->setPosition(slotPositions[items[i]->slot]);
	}
}
void ReorderableGrid::doLayout(int numCols) {
	slotPositions.clear();
	float itemWidth = (width - (numCols - 1) * spacingAmt) / numCols;

	auto numRows = items.size() / numCols;
	if (items.size() % numCols != 0) {
		numRows += 1;
	}
	float itemHeight = (height - (numRows - 1) * spacingAmt) / numRows;
	for (int i = 0; i < items.size(); i++) {
		int xx = i % numCols;
		int yy = i / numCols;
		slotPositions.push_back(
			{xx * (itemWidth + spacingAmt), yy * (itemHeight + spacingAmt), itemWidth, itemHeight});
	}

	for (int i = 0; i < slotPositions.size(); i++) {
		items[i]->setPosition(slotPositions[items[i]->slot]);
	}
}

void ReorderableGrid::draw() {
	ScopedAlphaBlend bl(g, true);
	g.pushMatrix();
	g.translate(x, y);

	for (auto &sl: slotPositions) {
		drawSlotPosition(sl);
	}

	// draw all non-dragging items
	for (auto *item: items) {
		if (!item->dragging) {
			item->draw(g, reordering);
		}
	}

	// draw all dragging items
	for (auto *item: items) {
		if (item->dragging) {
			item->draw(g, reordering);
		}
	}
	g.popMatrix();
}

bool ReorderableGrid::touchDown(float x, float y, int id) {
	if (!reordering) return false;
	x -= this->x;
	y -= this->y;
	for (auto *item: items) {
		if (item->inside(x, y)) {
			item->startDragging({x, y}, id);
			return true;
		}
	}
	return false;
}

std::vector<int> ReorderableGrid::getSlots() {
	std::vector<int> sl;
	for (int slot = 0; slot < items.size(); slot++) {
		for (int i = 0; i < items.size(); i++) {
			if (slot == items[i]->slot) {
				sl.push_back(items[i]->index);
				break;
			}
		}
	}
	return sl;
}

void ReorderableGrid::touchMoved(float x, float y, int id) {
	if (!reordering) return;
	x -= this->x;
	y -= this->y;
	// mapping from index to new slot pos
	std::map<int, int> moves;
	for (auto *item: items) {
		if (item->dragging && item->touchId == id) {
			int move = item->update(slotPositions, x, y);
			if (move != -1) {
				moves[item->index] = move;
			}
		}
	}

	if (moves.size() > 0) {
		std::vector<int> newSlots(items.size(), -1);
		for (auto &m: moves) {
			newSlots[m.second] = m.first;
		}
		std::vector<int> nonDraggingSlots = getSlots();
		for (int i = 0; i < nonDraggingSlots.size(); i++) {
			if (moves.find(nonDraggingSlots[i]) != moves.end()) {
				nonDraggingSlots.erase(nonDraggingSlots.begin() + i);
				i--;
			}
		}

		int next = 0;

		for (int i = 0; i < newSlots.size(); i++) {
			if (newSlots[i] == -1) {
				newSlots[i] = nonDraggingSlots[next++];
			}
		}

		for (int i = 0; i < newSlots.size(); i++) {
			items[newSlots[i]]->setTargetSlot(slotPositions, i);
		}
		if (rearranged) rearranged(newSlots);
	}
}

void ReorderableGrid::touchUp(float x, float y, int id) {
	if (!reordering) return;
	x -= this->x;
	y -= this->y;
	for (auto *item: items) {
		if (item->dragging && item->touchId == id) {
			item->stopDragging(slotPositions);
		}
	}
}

// override this to do a custom slot position.
void ReorderableGrid::drawSlotPosition(const Rectf &r) {
	if (reordering) {
		g.setColor(0, 0, 0, 0.1);
		g.fill();
		g.drawRoundedRect(r, 30);
	}
}
