//
//  Scroller.h
//
//
//

#pragma once
#include "Layer.h"
#include <deque>
#include <utility>
class Scroller : public Layer {
public:
	Scroller(Graphics &g);

	void addContent(Layer *layer);
	void clear() override;

	void draw() override;

	void touchOver(float x, float y) override;

	void touchUp(float x, float y, int id) override;

	void touchMoved(float x, float y, int id) override;

	bool touchDown(float x, float y, int id) override;

	bool mouseScrolled(float x, float y, float scrollX, float scrollY) override;

	bool drawingScrollbar = false;
	bool scrollbarDims	  = true;

	// call this if the height of the content is modified
	// if will only look at the last content item's bottom edge
	void contentUpdated();

	// call this if you want to force a different height than the actual contents.
	void setContentHeight(float contentHeight);
	virtual void drawSelfAndChildren() override;
	vec4 color {1.f, 1.f, 1.f, 1.f};

protected:
	// once-per-frame scroll physics, driven by the ScopedUpdater below
	virtual void onUpdate();

	void drawScrollbar();
	Layer *content;
	ScopedUpdater updater {g, [this] { onUpdate(); }};
	bool scrolling = false;
	glm::vec2 lastTouch {0, 0};
	// px / sec
	glm::vec2 contentVelocity {0, 0};
	bool contentHeightExplicitlySet = false;
	float scrollbarActivityAmt		= 0;

	// (time, content->y) samples for release-velocity estimation
	std::deque<std::pair<double, float>> velocitySamples;

	// drag anchor — finger position is mapped through the rubber-band from this anchor
	float dragAnchorContentY = 0.f;
	float dragAnchorTouchY	 = 0.f;
};
