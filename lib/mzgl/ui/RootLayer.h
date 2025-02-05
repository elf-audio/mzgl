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
	RootLayer(Graphics &g)
		: Layer(g, "root") {
		debugOverlay = new DebugOverlay(g);

		addChild(debugOverlay);
	}

	void enableDebug(bool debug) { debugOverlay->interactive = debugOverlay->visible = debug; }
	bool isDebugging() { return debugOverlay->visible; }
	virtual ~RootLayer() {}

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

private:
	DebugOverlay *debugOverlay;
};
