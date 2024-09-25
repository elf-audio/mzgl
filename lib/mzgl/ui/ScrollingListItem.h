#pragma once

class Drawer;
#include "Layer.h"

// this holds the data. subclass this to hold extra data if needed
class ScrollingListItem {
public:
	std::string name;
	bool selected = false;
	explicit ScrollingListItem(std::string name)
		: name(name) {}
	virtual ~ScrollingListItem() = default;
};

// subclass this view to make your own item view
class ScrollingListItemView : public Layer {
public:
	vec4 selectedColor {0.4f, 0.4f, 0.4f, 1.f};
	vec4 unselectedColor {0.1f, 0.1f, 0.1f, 1.f};
	vec4 dividerColor {0.5, 0.5, 0.5, 1.f};

	bool hasDivider = true;
	bool hasBG		= true;

	// your subclass should respond to selected when drawing
	// TODO: make this happen on a function call rather than variable
	// so we can trigger animations.

	std::shared_ptr<ScrollingListItem> item;
	ScrollingListItemView(Graphics &g, std::shared_ptr<ScrollingListItem> item)
		: Layer(g)
		, item(item) {
		interactive = true;
	}
	virtual ~ScrollingListItemView() {}

	virtual bool hasCustomHeight() { return false; }
	virtual int getCustomHeight() { return 0; }

	// override draw() to draw anything that can't be drawn with draw(Drawer&)
	virtual void draw() override {}
	virtual void draw(Drawer &d);

	// can select yourself - not totally happy with this
	std::function<void()> selectedSelf;
	std::function<void()> deleteSelf;

	// gets called if you hold your finger on the list for too long and don't
	// move it - useful for dragging.
	virtual void touchHeld(Layer *list, float x, float y, int id) {}

	bool collapsing = false;
};

class ScrollingListStringView : public ScrollingListItemView {
public:
	ScrollingListStringView(Graphics &g, std::shared_ptr<ScrollingListItem> item)
		: ScrollingListItemView(g, item) {}
	void draw() override;
};
