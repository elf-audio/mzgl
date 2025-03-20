//
//  Renderer.hpp
//  MZGL
//
//  Created by Marek Bereza on 15/01/2018.
//  Copyright © 2018 Marek Bereza. All rights reserved.
//

#pragma once

#include <glm/glm.hpp>
#include "Vbo.h"
#include "Rectf.h"
#include "Fbo.h"
#include "MatrixStack.h"
#include "Shader.h"
#include "Font.h"
#include <map>
#include <functional>

#define MZ_KEY_LEFT		 256
#define MZ_KEY_RIGHT	 257
#define MZ_KEY_DOWN		 258
#define MZ_KEY_UP		 259
#define MZ_KEY_DELETE	 127 // this is actually ascii, but non-printable
#define MZ_KEY_TAB		 9
#define MZ_KEY_ESCAPE	 27
#define MZ_KEY_SHIFT_TAB 25
#define MZ_KEY_RETURN	 13
#define MZ_KEY_ENTER	 3
#define MZ_KEY_SHIFT	 1001
#define MZ_KEY_FN		 1002
#define MZ_KEY_CTRL		 1002
#define MZ_KEY_ALT		 1003
#define MZ_KEY_CMD		 1004

// these aren't keys as such but if there's
// 1 rotary encoder then this is the message
// they send.
#define MZ_KEY_INCREMENT 2099
#define MZ_KEY_DECREMENT 2098

class Layer;
class App;

/////////////////////////////////////////////////////////////////////////////
// COLORS
bool isHexColour(std::string s);
bool isRGBColour(std::string s);
bool isRGBAColour(std::string s);

glm::vec4 hexColor(int hex, float a = 1);
glm::vec4 hexColor(std::string s);

glm::vec4 svgHexColor(std::string s);
glm::vec4 namedColor(std::string s);

glm::vec4 rgbColor(std::string s);
glm::vec4 rgbaColor(std::string s);

glm::vec3 rgb2hsv(glm::vec4 rgb);
glm::vec4 hsv2rgb(glm::vec3 hsv);
class ScopedAlphaBlend;
class ScopedNoFill;
class ScopedTranslate;
class ScopedTransform;
class GraphicsAPI;
class OpenGLAPI;
class Graphics {
public:
	Graphics();
	~Graphics();
	int width			 = 0;
	int height			 = 0;
	float pixelScale	 = 2.f;
	double frameDelta	 = 1.0 / 60.0;
	double currFrameTime = 0.0;
	uint32_t frameNum	 = 0;

	void setColor(float r, float g, float b, float a = 1.f);
	void setColor(glm::vec3 c);
	void setColor(glm::vec4 c);
	void setColor(glm::vec4 c, float alpha);
	void setColor(float bri);
	void setHexColor(int hex, float a = 1.f);
	void saveScreen(std::string pngPath);

	std::function<void(bool)> setAntialiasing = [](bool) {};

	enum class BlendMode {
		Alpha,
		Additive,
		Multiply,
		Screen,
		Subtract,
	};

	void setBlending(bool shouldBlend);
	void setBlendMode(BlendMode blendMode);
	[[nodiscard]] BlendMode getBlendMode() const;
	[[nodiscard]] bool isBlending() const;
	[[nodiscard]] glm::vec4 getColor() const;

	void clear(float c);
	void clear(float r, float g, float b, float a = 1.f);
	void clear(glm::vec4 c);
	void clear(glm::vec3 c);

	Shader *currShader = nullptr;

	// these are the default shaders
	ShaderRef nothingShader;
	ShaderRef colorShader;
	ShaderRef colorTextureShader;
	ShaderRef texShader;
	ShaderRef fontShader;

	void loadDefaultShaders();

	void noFill();
	void fill();
	bool isFilling();

	void setStrokeWeight(float f);
	float getStrokeWeight();

	/////////////////////////////////////////////////////////////////////////////
	// SHAPES

	void drawRect(float x, float y, float width, float height);
	void drawRect(const Rectf &r);
	void draw(const Rectf &r);

	// this is very slow - use the RoundedRect class instead to cache
	void drawRoundedRect(const Rectf &r, float radius);
	void drawRoundedRectShadow(Rectf r, float radius, float shadow);
	void draw(const Rectf &r, float radius);

	void drawCircle(float x, float y, float r);
	void drawCircle(glm::vec2 c, float r);
	void drawArc(glm::vec2 c, float r, float startAngle, float endAngle);
	void drawLine(glm::vec2 a, glm::vec2 b);
	void drawLine(float x1, float y1, float x2, float y2);
	void drawLineStrip(const std::vector<vec2> &pts);
	void drawPlus(vec2 c, int diameter, int thickness);

	void drawCross(vec2 c, int diameter, int thickness);

	void drawChevronLeft(vec2 c, int radius, int thickness);
	void drawChevronRight(vec2 c, int radius, int thickness);
	void drawChevronUp(vec2 c, int radius, int thickness);
	void drawChevronDown(vec2 c, int radius, int thickness);

	void drawShape(const std::vector<vec2> &shape);
	void drawTriangle(vec2 a, vec2 b, vec2 c);
	void drawVerts(const std::vector<glm::vec2> &verts, Vbo::PrimitiveType type = Vbo::PrimitiveType::Triangles);
	void drawVerts(const std::vector<glm::vec2> &verts,
				   const std::vector<glm::vec4> &cols,
				   Vbo::PrimitiveType type = Vbo::PrimitiveType::Triangles);

	void drawText(const std::string &s, float x, float y);
	void drawText(const std::string &s, glm::vec2 p);
	void drawTextWithBG(const std::string &s, vec4 bgColor, float x, float y);
	void drawTextCentred(const std::string &s, glm::vec2 c);
	void drawTextVerticallyCentred(const std::string &text, glm::vec2 c);
	void drawTextHorizontallyCentred(const std::string &text, glm::vec2 c);

	/////////////////////////////////////////////////////////////////////////////
	// MATRIX STUFF
	void pushMatrix();
	void popMatrix();

	void loadIdentity();
	void translate(glm::vec2 d);
	void translate(glm::vec3 d);

	void translate(float x, float y, float z = 0);
	void scale(float amt);
	void scale(float x, float y, float z = 1);

	void rotate(float angle, glm::vec3 axis);
	void rotateX(float angle);
	void rotateY(float angle);
	void rotateZ(float angle);
	void setProjectionMatrix(glm::mat4 projMat);

	void setViewMatrix(glm::mat4 viewMat);
	glm::mat4 getMVP();
	const glm::mat4 &getModelMatrix();

	void maskOn(const Rectf &r);
	void maskOff();
	bool isMaskOn();
	Rectf getMaskRect();

	/////////////////////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////
	// PRIVATE
	void initGraphics();
	void setupView(bool flipped = true, int w = 0, int h = 0);
	void setupViewOrtho(float w = 0.f, float h = 0.f);
	unsigned int getFrameNum() { return frameNum; }
	// when you want to unbind a frame buffer (fbo) you need to bind to this
	// as on iOS, the default framebuffer is not always 0

	Font &getFont();
	void deallocateFont();
	void reloadFont();
	Font *font = nullptr;
	std::vector<unsigned char> getDefaultFontTTFData();

	bool firstFrame = true;

	// this function will warp a rect so it works with glScissor
	// - different OS's have different quirks regarding this
	void warpMaskForScissor(Rectf &a);

	// currently focused layer once a touch has gone down
	// this should be replaced by a touch id to Layer* map for multitouch
	// needs to be here so it's not static
	std::map<int, Layer *> focusedLayers;

	Layer *keyboardFocusedLayer = nullptr;

	// OpenGL only
	int32_t getDefaultFrameBufferId();

private:
	float strokeWeight	 = 1;
	BlendMode blendMode	 = BlendMode::Alpha;
	bool blendingEnabled = false;
	bool filling		 = true;
	glm::vec4 color;

	MatrixStack modelMatrixStack;
	glm::mat4 projectionMatrix;
	glm::mat4 viewMatrix;
	// this is cached version of the above multiplied
	glm::mat4 viewProjectionMatrix;
	friend class ScopedAlphaBlend;
	friend class ScopedTranslate;

	std::unique_ptr<GraphicsAPI> api;
};

template <Graphics::BlendMode newBlendMode>
class ScopedBlend {
public:
	Graphics &g;
	ScopedBlend(Graphics &g)
		: g(g) {
		originalBlendState = g.isBlending();
		originalBlendMode  = g.getBlendMode();
		g.setBlending(true);
		if (originalBlendMode != newBlendMode) {
			g.setBlendMode(newBlendMode);
		}
	}

	virtual ~ScopedBlend() {
		if (!originalBlendState) {
			g.setBlending(originalBlendState);
		}
		if (originalBlendMode != newBlendMode) {
			g.setBlendMode(originalBlendMode);
		}
	}

private:
	bool originalBlendState;
	Graphics::BlendMode originalBlendMode;
};

using ScopedAdditiveBlend = ScopedBlend<Graphics::BlendMode::Additive>;

class ScopedAlphaBlend {
public:
	Graphics &g;
	ScopedAlphaBlend(Graphics &g, bool shouldBlend);
	virtual ~ScopedAlphaBlend();

private:
	bool originalBlendState;
};

class ScopedFill {
public:
	ScopedFill(Graphics &_graphics);
	~ScopedFill();

private:
	Graphics &graphics;
	const bool isFilling;
};

class ScopedNoFill {
public:
	ScopedNoFill(Graphics &_graphics);
	~ScopedNoFill();

private:
	Graphics &graphics;
	const bool isFilling;
};

class ScopedTranslate {
public:
	Graphics &g;
	ScopedTranslate(Graphics &g, glm::vec2 p)
		: g(g) {
		g.pushMatrix();
		g.translate(p.x, p.y);
	}
	ScopedTranslate(Graphics &g, float x, float y)
		: g(g) {
		g.pushMatrix();
		g.translate(x, y);
	}
	virtual ~ScopedTranslate() { g.popMatrix(); }
};
class ScopedTransform {
public:
	Graphics &g;
	ScopedTransform(Graphics &g)
		: g(g) {
		g.pushMatrix();
	}
	virtual ~ScopedTransform() { g.popMatrix(); }
};

class ScopedShaderEnable {
public:
	ShaderRef sh;
	ScopedShaderEnable(ShaderRef shader)
		: sh(shader) {
		sh->begin();
	}
	virtual ~ScopedShaderEnable() { sh->end(); }
};

#include "ScopedMask.h"
