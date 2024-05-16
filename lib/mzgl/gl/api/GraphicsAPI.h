
#pragma once

#include "Graphics.h"

class GraphicsAPI {
public:
	GraphicsAPI(Graphics &g)
		: g(g) {}

	virtual ~GraphicsAPI()									 = default;
	virtual void init()										 = 0;
	virtual void setBlending(bool shouldBlend)				 = 0;
	virtual void setLineWidth(float f)						 = 0;
	virtual void setBlendMode(Graphics::BlendMode blendMode) = 0;
	virtual void clear(vec4 c)								 = 0;

	virtual void maskOn(const Rectf &r) = 0;
	virtual void maskOff()				= 0;
	[[nodiscard]] virtual bool isMaskOn() const				= 0;
	[[nodiscard]] virtual Rectf getMaskRect() const			= 0;

	virtual void readScreenPixels(std::vector<uint8_t> &outData, const Rectf &r) = 0;

	virtual void drawVerts(const std::vector<glm::vec2> &verts, Vbo::PrimitiveType type) = 0;

	virtual void drawVerts(const std::vector<glm::vec2> &verts,
						   const std::vector<glm::vec4> &cols,
						   Vbo::PrimitiveType type) = 0;

protected:
	Graphics &g;
};