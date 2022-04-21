//
//  RearrangableGrid.h
//  mzgl
//
//  Created by Marek Bereza on 24/06/2020.
//  Copyright © 2020 Marek Bereza. All rights reserved.
//

#pragma once



#include "ReorderableItem.h"


class ReorderableGrid : public Layer {
public:
	std::vector<Rectf> slotPositions;
	std::vector<ReorderableItem*> items;
	std::function<void(const std::vector<int> &)> rearranged;
	
	// this is how much padding there is between
	// each slotPosition.
	int spacingAmt = 20;
	
	ReorderableGrid(Graphics &g);
	
	void addItem(ReorderableItem *item);

	void setOrder(const std::vector<int> &order);
	
	bool isReordering();
	
	void setReordering(bool reordering);
	
	void doLayout(int numCols);
	void doLayoutWithPositions(const std::vector<Rectf> &newSlots);
	
	void draw() override;
	
	bool touchDown(float x, float y, int id) override;
	
	std::vector<int> getSlots();
	
	
	void touchMoved(float x, float y, int id) override;
	
	void touchUp(float x, float y, int id) override;
protected:
	bool reordering = false;
	
	// override this to do a custom slot position.
	virtual void drawSlotPosition(const Rectf &r);
};


