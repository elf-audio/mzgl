//
//  Scroller.h
//
//
//

#pragma once
#include "Layer.h"
class Scroller: public Layer {
public:
	
	Scroller(Graphics &g);
	
	void addContent(Layer *layer);
	void clear() override;
	void update() override;
	
	void draw() override;
	
	void touchOver(float x, float y) override;
	
	void touchUp(float x, float y, int id) override;
	
	void touchMoved(float x, float y, int id) override;
	
	bool touchDown(float x, float y, int id) override;
	
	void mouseScrolled(float x, float y, float scrollX, float scrollY ) override;
	
	bool drawingScrollbar = false;
	bool scrollbarDims = true;
	
	// call this if the height of the content is modified
	// if will only look at the last content item's bottom edge
	void contentUpdated();
	
	// call this if you want to force a different height than the actual contents.
	void setContentHeight(float contentHeight);
	virtual void _draw() override;
protected:
	
	void drawScrollbar();
	Layer *content;
	bool scrolling = false;
	glm::vec2 lastTouch {0, 0};
	glm::vec2 contentVelocity {0, 0};
	bool contentHeightExplicitlySet = false;
	float scrollbarActivityAmt = 0;
};


