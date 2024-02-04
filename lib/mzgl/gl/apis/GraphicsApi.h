
#pragma once

#include <glm/glm.hpp>
#include "GraphicsState.h"
enum class BlendMode {
	Alpha, // the classic alpha blending mode
	Additive,
};

class GraphicsApi {
public:
	GraphicsApi(GraphicsState &state)
		: state(state) {}
	virtual ~GraphicsApi() {}
	// TODO: rename to init()
	virtual void initGraphics()											= 0;
	virtual void clear(const vec4 &bgColor)								= 0;
	virtual void saveScreen(std::string pngPath, int width, int height) = 0;

	virtual void setBlendMode(BlendMode blendMode) = 0;

	virtual void setBlending(bool shouldBlend) = 0;

	virtual void setStrokeWeight(float f) = 0;

	virtual void maskOn(const Rectf &r) = 0;

	virtual void maskOff() = 0;

	virtual bool isMaskOn() = 0;

    virtual void drawVerts(const std::vector<glm::vec2> &verts, Vbo::PrimitiveType type = Vbo::PrimitiveType::Triangles) = 0;
    virtual void drawVerts(const std::vector<glm::vec2> &verts, const std::vector<uint32_t> &indices) = 0;
    virtual void drawVerts(const std::vector<glm::vec2> &verts,
                   const std::vector<glm::vec4> &cols,
                   Vbo::PrimitiveType type = Vbo::PrimitiveType::Triangles) = 0;

protected:
	GraphicsState &state;
};