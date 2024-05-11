
#pragma once

class GraphicsAPI {
public:
	GraphicsAPI(Graphics &g)
		: g(g) {}
	Graphics &g;
	virtual ~GraphicsAPI()									 = default;
	virtual void init()										 = 0;
	virtual void setBlending(bool shouldBlend)				 = 0;
	virtual void setLineWidth(float f)						 = 0;
	virtual void setBlendMode(Graphics::BlendMode blendMode) = 0;
	virtual void clear(vec4 c)								 = 0;

	virtual void maskOn(const Rectf &r) = 0;
	virtual void maskOff()				= 0;
	virtual bool isMaskOn()				= 0;
	virtual Rectf getMaskRect()			= 0;

	virtual void readScreenPixels(uint8_t *data, const Rectf &r) = 0;

	virtual void drawVerts(const std::vector<glm::vec2> &verts, Vbo::PrimitiveType type) = 0;

	virtual void drawVerts(const std::vector<glm::vec2> &verts, const std::vector<uint32_t> &indices) = 0;

	virtual void drawVerts(const std::vector<glm::vec2> &verts,
						   const std::vector<glm::vec4> &cols,
						   Vbo::PrimitiveType type) = 0;
};