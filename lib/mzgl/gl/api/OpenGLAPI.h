#pragma once

#include "GraphicsAPI.h"

class OpenGLAPI : public GraphicsAPI {
public:
	OpenGLAPI(Graphics &g)
		: GraphicsAPI(g) {}

	void init() override;

	void setLineWidth(float f) override;
	void setBlending(bool shouldBlend) override;

	void drawVerts(const std::vector<glm::vec2> &verts, Vbo::PrimitiveType type) override;

	void drawVerts(const std::vector<glm::vec2> &verts,
				   const std::vector<glm::vec4> &cols,
				   Vbo::PrimitiveType type) override;

	void maskOn(const Rectf &r) override;

	void maskOff() override;

	bool isMaskOn() override;

	Rectf getMaskRect() override;

	void readScreenPixels(uint8_t *data, const Rectf &r) override;
	void clear(vec4 c) override;
	void setBlendMode(Graphics::BlendMode blendMode) override;

	int32_t getDefaultFrameBufferId();

private:
	void loadDefaultShaders();
	int32_t defaultFBO;
	uint32_t immediateVertexArray  = 0;
	uint32_t immediateVertexBuffer = 0;
	uint32_t immediateColorBuffer  = 0;
	uint32_t immediateIndexBuffer  = 0;
};
