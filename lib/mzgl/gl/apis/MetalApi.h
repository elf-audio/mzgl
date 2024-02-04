
#pragma once

#include "GraphicsApi.h"

class MetalApi : public GraphicsApi {
public:
	MetalApi(GraphicsState &state)
		: GraphicsApi(state) {}

	virtual ~MetalApi() {}
	// TODO: rename to init()
	virtual void initGraphics() {}
	virtual void clear(const vec4 &bgColor) {}
	virtual void saveScreen(std::string pngPath, int width, int height) {}

	virtual void setBlendMode(BlendMode blendMode) {}

	virtual void setBlending(bool shouldBlend) {}

	virtual void setStrokeWeight(float f) {}

	virtual void maskOn(const Rectf &r) {}

	virtual void maskOff() {}

	virtual bool isMaskOn() { return false; }
};