//
//  Scroller.h
//
//
//

#pragma once
#include "Layer.h"
#include <deque>
#include <functional>
#include <utility>
class Scroller : public Layer {
public:
	Scroller(Graphics &g);

	void addContent(Layer *layer);
	void clear() override;

	// ---- pull-to-reveal header (opt-in) ------------------------------------
	// Give the scroller a header layer that sits hidden just above the top and
	// is revealed by over-pulling the list downward (the classic mobile
	// pull-to-search gesture). Pull past ~half the header height and release to
	// "pin" it open; a smaller pull springs back and re-hides it. The Scroller
	// takes ownership of `header`, and draws + hit-tests it itself (it is not a
	// normal child), so this works for any Scroller subclass (e.g.
	// ScrollingList) without touching that subclass's own draw/touch logic.
	void setPullToRevealHeader(Layer *header, float headerHeight);
	void setRevealHeaderHeight(float headerHeight) { revealHeaderHeight = headerHeight; }
	Layer *getRevealHeader() const { return revealHeader; }
	bool isRevealPinned() const { return revealPinned; }
	void setRevealPinned(bool pinned);

	// fired when the header pins open (e.g. to focus a search field) / unpins.
	std::function<void()> onRevealPinned;
	std::function<void()> onRevealUnpinned;

	// Optional: return true if a pinned header should collapse (un-pin and
	// scroll away with the list) when the user starts scrolling. e.g. a search
	// field returns true when empty — no query to keep, so let it scroll off.
	std::function<bool()> revealCollapseOnScroll;

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

	// ---- pull-to-reveal state ----------------------------------------------
	// revealHeader is a normal interactive child (added last, so the layer
	// system routes touches to it before the list content / cells behind it).
	Layer *revealHeader		 = nullptr;
	float revealHeaderHeight = 0.f;
	float revealAmt			 = 0.f; // 0..revealHeaderHeight currently on-screen
	bool revealPinned		 = false;
	bool revealSettling		 = false; // animating content->y to the top limit

	// resting top offset for content->y: 0 normally, headerHeight when pinned.
	float topLimit() const { return revealPinned ? revealHeaderHeight : 0.f; }
	void updateReveal();	 // recompute revealAmt + position the header child
	void drawRevealHeader(); // draw header explicitly (subclasses with custom draw)
};
