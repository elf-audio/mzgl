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
	std::function<ScrollingListItemView *(std::shared_ptr<ScrollingListItem> item)> getNewListItem =
		[this](std::shared_ptr<ScrollingListItem> item) -> ScrollingListItemView * {
		return new ScrollingListStringView(g, item);
	};

	ScrollingList(Graphics &g, float itemHeight = 60);

	std::function<void(int)> itemSelected = [](int) {};
	std::function<void(int)> itemFocused  = [](int) {};

	// only for row types that are deletable
	std::function<void(std::shared_ptr<ScrollingListItem>)> itemDeleted;
	void setItems(const std::vector<std::shared_ptr<ScrollingListItem>> &items);

	// ---- search / filtering (datatype-agnostic) ----------------------------
	// setItems() stores the full set; the visible rows are re-derived from the
	// current query via searchMatches. Works for any item type — subclass
	// ScrollingListItem to carry your data and (optionally) override
	// searchMatches to match whatever fields you like.
	void setSearchQuery(const std::string &query);
	const std::string &getSearchQuery() const { return searchQuery; }
	// how one item is tested against the (non-empty) query. Default:
	// case-insensitive substring match on item->name.
	std::function<bool(const std::shared_ptr<ScrollingListItem> &item, const std::string &query)>
		searchMatches;

	void unselect();
	std::shared_ptr<ScrollingListItem> getSelectedItem();
	std::shared_ptr<ScrollingListItem> getItem(int index);
	void setItemHeight(float itemHeight);

	//////////////////////////////////////////////////////////
	bool touchDown(float x, float y, int id) override;

	void touchMoved(float x, float y, int id) override;

	void touchUp(float x, float y, int id) override;

	void setEmptyMessage(Layer *emptyMessageLayer);

	// if you don't want to call touchUp because it might trigger
	// a selection event, call this
	void cancelTouches();

	void doLayout() override;

	void select(int itemIndex);
	void select(std::shared_ptr<ScrollingListItem> item);
	int getNumItems() const { return static_cast<int>(items.size()); }
	int getSelectedIndex() const { return selectedIndex; }
	float getContentHeight() { return content->height; }
	//	void draw() override;
	virtual void drawSelfAndChildren() override;
	bool empty() const { return items.empty(); }

	virtual bool keyDown(int key) override;

	auto begin() { return items.begin(); }
	auto end() { return items.end(); }

protected:
	void onUpdate() override;
	int selectedIndex = -1;
	void focus(int index);
	void touchHeld();
	bool selectionIsFocus = false;

	int touchingId = -1;

	bool canSelect = true;
	void updateItems();
	bool selecting = false;
	vec2 startTouch;
	Rectf touchRect;
	float touchDownTime	 = -1;
	bool touchHeldCalled = false;
	VboRef vbo;
	void collapseAndDeleteCell(ScrollingListItemView *collapsingCell);
	std::vector<ScrollingListItemView *> collapsingCells;

private:
	std::vector<std::shared_ptr<ScrollingListItem>> allItems; // full, unfiltered set
	std::vector<std::shared_ptr<ScrollingListItem>> items;	  // currently visible (filtered)
	std::string searchQuery;
	void applyFilter(); // rebuild `items` from allItems + searchQuery, then relayout
	Layer *emptyMessageLayer = nullptr;
	Drawer d;
};
