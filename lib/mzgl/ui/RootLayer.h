//
//  RootLayer.h
//  MZGL
//
//  Created by Marek Bereza on 23/01/2018.
//  Copyright © 2018 Marek Bereza. All rights reserved.
//

#pragma once
#include "Layer.h"
#include "events.h"
class RootLayer : public Layer {
public:
	RootLayer(Graphics &g) : Layer(g, "root") {
		color.a = 0.f;
	}
	virtual ~RootLayer() {}
	
	void drawDebug(Layer *layer = nullptr) {
		if(layer==nullptr) {
			layer = this;
		}
		for(int i = 0; i < layer->getNumChildren(); i++) {
			drawDebug(layer->getChild(i));
		}
		if(visible) {
			Rectf r = layer->getAbsoluteRect();
			g.fill();
			g.setColor(0.5, 0, 0, 0.2);
			g.drawRect(r);
			g.noFill();
			g.setColor(1, 1, 1, 0.5);
			g.drawRect(r);
		}
	}
	
//`	
//#ifdef __APPLE__
//#if !TARGET_OS_IOS
//	// JUST FOR IMGUI
//	function<bool(void)> isMouseInteractionEnabled = []() { return true; };
//
//	void _touchUp(float x, float y, int id) override {
//		if(isMouseInteractionEnabled()) Layer::_touchUp(x, y, id);
//	}
//	void _touchMoved(float x, float y, int id) override {
//		if(isMouseInteractionEnabled()) Layer::_touchMoved(x, y, id);
//	}
//	
//	bool _touchDown(float x, float y, int id) override {
//		if(isMouseInteractionEnabled()) return Layer::_touchDown(x, y, id);
//		return false;
//	}
//	void _mouseScrolled(float x, float y, float scrollX, float scrollY ) override {
//		if(isMouseInteractionEnabled()) Layer::_mouseScrolled(x, y, scrollX, scrollY);
//	}
//	void _mouseZoomed(float x, float y, float zoom) override {
//		if(isMouseInteractionEnabled()) Layer::_mouseZoomed(x, y, zoom);
//	}
//#endif
//#endif`
};

