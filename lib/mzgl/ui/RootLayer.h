//
//  RootLayer.h
//  MZGL
//
//  Created by Marek Bereza on 23/01/2018.
//  Copyright © 2018 Marek Bereza. All rights reserved.
//

#pragma once

#include "Layer.h"
#include "DebugOverlay.h"

class RootLayer : public Layer {
public:
	explicit RootLayer(Graphics &g)
		: Layer(g, "root") {
		debugOverlay = new DebugOverlay(g);
		addChild(debugOverlay);
	}

	~RootLayer() override = default;

	void setDebugOverlayCallback(const std::function<void(std::string, std::string)> &callback) {
		debugOverlay->onLayerChanged = callback;
	}
	void enableDebug(bool debug) { debugOverlay->interactive = debugOverlay->visible = debug; }
	bool isDebugging() { return debugOverlay->visible; }

	void drawDebug(Layer *layer = nullptr) {
		if (layer == nullptr) {
			layer = this;
		}
		for (int i = 0; i < layer->getNumChildren(); i++) {
			drawDebug(layer->getChild(i));
		}
		if (visible) {
			Rectf r = layer->getAbsoluteRect();
			g.fill();
			g.setColor(0.5f, 0.0f, 0.0f, 0.2f);
			g.drawRect(r);
			g.noFill();
			g.setColor(1.0f, 1.0f, 1.0f, 0.5f);
			g.drawRect(r);
		}
	}

	void _touchOver(float x, float y) override {
		Layer::_touchOver(x, y);
		g.drainDeferredActions();
	}

	void _touchUp(float x, float y, int id) override {
		Layer::_touchUp(x, y, id);
		g.drainDeferredActions();
	}

	void _touchMoved(float x, float y, int id) override {
		Layer::_touchMoved(x, y, id);
		g.drainDeferredActions();
	}

	bool _touchDown(float x, float y, int id) override {
		auto result = Layer::_touchDown(x, y, id);
		g.drainDeferredActions();
		return result;
	}

	bool _mouseScrolled(float x, float y, float scrollX, float scrollY) override {
		auto result = Layer::_mouseScrolled(x, y, scrollX, scrollY);
		g.drainDeferredActions();
		return result;
	}

	bool _mouseZoomed(float x, float y, float zoom) override {
		auto result = Layer::_mouseZoomed(x, y, zoom);
		g.drainDeferredActions();
		return result;
	}

	bool _keyDown(int key) override {
		auto result = Layer::_keyDown(key);
		g.drainDeferredActions();
		return result;
	}

	bool _keyUp(int key) override {
		auto result = Layer::_keyDown(key);
		g.drainDeferredActions();
		return result;
	}

private:
	DebugOverlay *debugOverlay;
};
