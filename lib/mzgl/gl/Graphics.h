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

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif
class Layer;
class App;



/////////////////////////////////////////////////////////////////////////////
// COLORS
glm::vec4 hexColor(int hex, float a = 1);
glm::vec4 hexColor(std::string s);

class ScopedAlphaBlend;
class ScopedTranslate;
class ScopedTransform;

class Graphics {
public:
	int width = 0;
	int height = 0;
	float pixelScale = 2.f;
	double frameDelta = 0.f;
	double currFrameTime = 0.f;
	
	void setColor(float r, float g, float b, float a = 1);
	void setColor(glm::vec3 c);
	void setColor(glm::vec4 c);
	void setColor(glm::vec4 c, float alpha);
	void setColor(float bri);
	void setHexColor(int hex, float a = 1);
	void saveScreen(std::string pngPath);

	std::function<void(bool)> setAntialiasing = [](bool){};
	
	void setBlending(bool shouldBlend);
	bool isBlending();
	glm::vec4 getColor();

	void clear(float c);
	void clear(float r, float g, float b, float a = 1);
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
	void drawVerts(const std::vector<glm::vec2> &verts, int type = GL_TRIANGLES);
	void drawVerts(const std::vector<glm::vec2> &verts, const std::vector<uint32_t> &indices);
	void drawVerts(const std::vector<glm::vec2> &verts, const std::vector<glm::vec4> &cols, int type = GL_TRIANGLES);

	void drawText(const std::string &s, float x, float y);
	void drawText(const std::string &s, glm::vec2 p);
	void drawTextCentred(const std:: string &s, glm::vec2 c);
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
	/////////////////////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////
	// PRIVATE
	void initGraphics();
	void setupView(bool flipped = true, int w = 0, int h = 0);
	void setupViewOrtho(float w = 0.f, float h = 0.f);
	unsigned int getFrameNum() { return frameNum; }
	// when you want to unbind a frame buffer (fbo) you need to bind to this
	// as on iOS, the default framebuffer is not always 0
	GLint getDefaultFrameBufferId();
	Font &getFont();
	void unloadFont();
	Font *font = nullptr;
	std::vector<unsigned char> getDefaultFontTTFData();
	
	
	bool firstFrame = true;

	// this function will warp a rect so it works with glScissor
	// - different OS's have different quirks regarding this
	void warpMaskForScissor(Rectf &a);

	// currently focused layer once a touch has gone down
	// this should be replaced by a touch id to Layer* map for multitouch
	// needs to be here so it's not static
	std::map<int,Layer*> focusedLayers;

	friend class App;
		
private:
	float strokeWeight = 1;
	GLint defaultFBO;
	bool blendingEnabled = false;
	bool filling = true;
	glm::vec4 color;
	//glm::mat4 mvp;
	MatrixStack modelMatrixStack;
	glm::mat4 projectionMatrix;
	glm::mat4 viewMatrix;
	// this is cached version of the above multiplied
	glm::mat4 viewProjectionMatrix;
	friend class ScopedAlphaBlend;
	friend class ScopedTranslate;
	
	GLuint immediateVertexArray = 0;
	GLuint immediateVertexBuffer = 0;
	GLuint immediateColorBuffer = 0;
	GLuint immediateIndexBuffer = 0;
	
	
	// was in Globals
	unsigned int frameNum = 0;
};

class ScopedAlphaBlend {
public:
	Graphics &g;
	ScopedAlphaBlend(Graphics &g, bool shouldBlend);
	virtual ~ScopedAlphaBlend();
private:
	bool originalBlendState;
	
	
};


struct ScopedTranslate {
	Graphics &g;
	ScopedTranslate(Graphics &g, glm::vec2 p) : g(g) { g.pushMatrix(); g.translate(p.x, p.y); }
	ScopedTranslate(Graphics &g, float x, float y) : g(g) { g.pushMatrix(); g.translate(x, y); }
	virtual ~ScopedTranslate() { g.popMatrix(); }
};
struct ScopedTransform {
	Graphics &g;
	ScopedTransform(Graphics &g) : g(g) { g.pushMatrix(); }
	virtual ~ScopedTransform() { g.popMatrix(); }
};

struct ScopedMask {
	bool scissorWasEnabled = false;
	float vals[4];
	bool masking = false;
	
	// you can initialize with empty constructor
	// so there's no mask actually happening
	// this is handy when you want conditinal masks
	ScopedMask() {
		masking = false;
	}
	ScopedMask(Graphics &g, const Rectf &r) {
		startMask(g, r);
	}

	void startMask(Graphics &g, const Rectf &r) {
		masking = true;
		if(glIsEnabled(GL_SCISSOR_TEST)) {
			scissorWasEnabled = true;
			glGetFloatv(GL_SCISSOR_BOX, vals);
		}
		glEnable(GL_SCISSOR_TEST);

#if defined(__linux__) && !defined(__ANDROID__)
		if(g.pixelScale!=1) {
			auto a = r;
			//g.warpMaskForScissor(a);

			glScissor(a.x, g.height-(a.bottom()), a.width, a.height);
		} else {
			glScissor(r.x, g.height-(r.bottom()), r.width, r.height);
		}
#else
		glScissor(r.x, g.height-(r.bottom()), r.width, r.height);
#endif
	}
	
	void stopMask() {
		if(!masking) return;
		if(scissorWasEnabled) {
			glScissor(vals[0], vals[1], vals[2], vals[3]);
		} else {
			glDisable(GL_SCISSOR_TEST);
		}
	}
	
	virtual ~ScopedMask() {
		stopMask();
	}
};
