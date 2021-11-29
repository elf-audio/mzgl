#pragma once

#include "Scroller.h"
#include "util.h"
#include "Drawer.h"
#include <memory>
#include "ScrollingListItem.h"







class OldScrollingList : public Scroller {
public:
	float itemHeight; // set in constructor
	
	// if you want to subclass ScrollingListItem for your own type of list item,
	// reassign this function to your own lambda returning your object.
	std::function<ScrollingListItemView*(std::shared_ptr<ScrollingListItem> item)> getNewListItem = [this](std::shared_ptr<ScrollingListItem> item) -> ScrollingListItemView* {
		return new ScrollingListItemView(g, item);
	};
	
	OldScrollingList(Graphics &g, float itemHeight = 60);
	std::function<void(int)> itemSelected;
	void setItems(const std::vector<std::shared_ptr<ScrollingListItem>> &items);
	void unselect();
	std::shared_ptr<ScrollingListItem> getSelectedItem();
	void setItemHeight(float itemHeight);
	
	//////////////////////////////////////////////////////////
	bool touchDown(float x, float y, int id) override;
	
	void touchMoved(float x, float y, int id) override;
	
	void touchUp(float x, float y, int id) override;
	
	void doLayout();

	int selectedIndex = -1;

//	void draw() override;
	void _draw() override;
	
protected:
	std::vector<std::shared_ptr<ScrollingListItem>> items;
	void updateItems();
	bool selecting = false;
	glm::vec2 startTouch;
	VboRef vbo;
	
};

