#pragma once

#include "Scroller.h"
#include "util.h"
#include "Drawer.h"
#include <memory>

#include "ScrollingListItem.h"

class ScrollingList : public Scroller {
public:
	float itemHeight; // set in constructor
	glm::vec4 bgColor {0.f, 0.f, 0.f, 1.f};
	// if you want to subclass ScrollingListItem for your own type of list item,
	// reassign this function to your own lambda returning your object.
	std::function<ScrollingListItemView*(std::shared_ptr<ScrollingListItem> item)> getNewListItem = [this](std::shared_ptr<ScrollingListItem> item) -> ScrollingListItemView* {
		return new ScrollingListStringView(g, item);
	};
	
	ScrollingList(Graphics &g, float itemHeight = 60);
	std::function<void(int)> itemSelected;
	
	// only for row types that are deletable
	std::function<void(std::shared_ptr<ScrollingListItem>)> itemDeleted;
	void setItems(const std::vector<std::shared_ptr<ScrollingListItem>> &items);
	void unselect();
	std::shared_ptr<ScrollingListItem> getSelectedItem();
	void setItemHeight(float itemHeight);
	
	//////////////////////////////////////////////////////////
	bool touchDown(float x, float y, int id) override;
	
	void touchMoved(float x, float y, int id) override;
	
	void touchUp(float x, float y, int id) override;
	
	// if you don't want to call touchUp because it might trigger
	// a selection event, call this
	void cancelTouches();
	
	void doLayout() override;

	int selectedIndex = -1;

	float getContentHeight() { return content->height; }
//	void draw() override;
	virtual void _draw() override;
	void update() override;
protected:
	
	void touchHeld();
	
	int touchingId = -1;

	bool canSelect = true;
	void updateItems();
	bool selecting = false;
	vec2 startTouch;
	Rectf touchRect;
	float touchDownTime = -1;
	bool touchHeldCalled = false;
	VboRef vbo;
	void collapseAndDeleteCell(ScrollingListItemView *collapsingCell);
	std::vector<ScrollingListItemView*> collapsingCells;
private:
	std::vector<std::shared_ptr<ScrollingListItem>> items;

};
