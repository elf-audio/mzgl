//
//  Graphics.cpp
//  MZGL
//
//  Created by Marek Bereza on 15/01/2018.
//  Copyright © 2018 Marek Bereza. All rights reserved.
//

#include <array>
#include "Graphics.h"
#include "Vbo.h"
#include "glm/gtc/matrix_transform.hpp"
#include "RoundedRect.h"
#include "Image.h"
#include "log.h"
#include "MitredLine.h"
#include "Drawer.h"
#include <utility>
#include "GraphicsAPI.h"

#include <array>
#include <utility>

void Graphics::init() {
	api->init();
}

Graphics::~Graphics() = default;

void Graphics::setBlending(bool shouldBlend) {
	// blend mode is already the same, jump out.
	if (shouldBlend == blendingEnabled) return;

	blendingEnabled = shouldBlend;
	api->setBlending(shouldBlend);
}

ScopedAlphaBlend::ScopedAlphaBlend(Graphics &g, bool shouldBlend)
	: g(g) {
	originalBlendState = g.blendingEnabled;
	g.setBlending(shouldBlend);
}

ScopedAlphaBlend::~ScopedAlphaBlend() {
	g.setBlending(originalBlendState);
}

ScopedNoFill::ScopedNoFill(Graphics &_graphics)
	: graphics {_graphics}
	, isFilling {graphics.isFilling()} {
	if (isFilling) {
		graphics.noFill();
	}
}

ScopedNoFill::~ScopedNoFill() {
	if (isFilling) {
		graphics.fill();
	}
}

ScopedFill::ScopedFill(Graphics &_graphics)
	: graphics {_graphics}
	, isFilling {graphics.isFilling()} {
	if (!isFilling) {
		graphics.fill();
	}
}

ScopedFill::~ScopedFill() {
	if (!isFilling) {
		graphics.noFill();
	}
}

bool Graphics::isBlending() const {
	return blendingEnabled;
}

glm::mat4 Graphics::getMVP() {
	return viewProjectionMatrix * modelMatrixStack.getMatrix();
}

void Graphics::initGraphics() {
	api->init();
	setBlending(true);
	setBlendMode(BlendMode::Alpha);
}

/////////////////////////////////////////////////////////////////////////////
// MATRIX STUFF

const glm::mat4 &Graphics::getModelMatrix() {
	return modelMatrixStack.getMatrix();
}
void Graphics::pushMatrix() {
	if (modelMatrixStack.size() > 100) {
		Log::w() << "WARNING: model matrix stack is over 100 deep";
	}
	modelMatrixStack.pushMatrix();
}
void Graphics::popMatrix() {
	modelMatrixStack.popMatrix();
}

void Graphics::loadIdentity() {
	modelMatrixStack.loadIdentity();
}

void Graphics::translate(glm::vec2 d) {
	translate(d.x, d.y);
}
void Graphics::translate(glm::vec3 d) {
	translate(d.x, d.y, d.z);
}
void Graphics::translate(float x, float y, float z) {
	modelMatrixStack.translate(x, y, z);
}
void Graphics::scale(float amt) {
	modelMatrixStack.scale(amt, amt, amt);
}

void Graphics::scale(float x, float y, float z) {
	modelMatrixStack.scale(x, y, z);
}

void Graphics::rotate(float angle, glm::vec3 axis) {
	modelMatrixStack.rotate(angle, axis);
}
void Graphics::rotateX(float angle) {
	modelMatrixStack.rotate(angle, glm::vec3(1, 0, 0));
}
void Graphics::rotateY(float angle) {
	modelMatrixStack.rotate(angle, glm::vec3(0, 1, 0));
}
void Graphics::rotateZ(float angle) {
	modelMatrixStack.rotate(angle, glm::vec3(0, 0, 1));
}

void Graphics::setProjectionMatrix(glm::mat4 projMat) {
	projectionMatrix	 = projMat;
	viewProjectionMatrix = projectionMatrix * viewMatrix;
}

void Graphics::setViewMatrix(glm::mat4 viewMat) {
	viewMatrix			 = viewMat;
	viewProjectionMatrix = projectionMatrix * viewMatrix;
}

void Graphics::setupViewOrtho(float w, float h) {
	if (w == 0) w = width;
	if (h == 0) h = height;
	if (w == 0 || h == 0) {
		Log::e() << "ERROR: height or width = 0";
		width = w = 100;
		height = h = 100;
	}

	projectionMatrix	 = glm::ortho(0.f, w, h, 0.f, -1000.f, 1000.f);
	viewMatrix			 = glm::mat4(1.f);
	viewProjectionMatrix = projectionMatrix * viewMatrix;
}

void Graphics::setupView(bool flipped, int w, int h) {
	if (w == 0) w = width;
	if (h == 0) h = height;
	if (w == 0 || h == 0) {
		Log::e() << "ERROR: height or width = 0";
		width = w = 100;
		height = h = 100;
	}
	// FROM OF
	float fov = 45.0f;

	// Camera matrix
	float eyeX	  = w / 2.f;
	float eyeY	  = h / 2.f;
	float halfFov = M_PI * fov / 360;
	float theTan  = tanf(halfFov);
	float dist	  = eyeY / theTan;

	float nearDist = dist / 10.0f;
	float farDist  = dist * 10.0f;

	projectionMatrix = glm::perspective(glm::radians(fov), w / (float) h, nearDist, farDist);
	viewMatrix =
		glm::lookAt(glm::vec3(eyeX, eyeY, dist * (flipped ? -1 : 1)), // Camera is at (4,3,-3), in World Space
					glm::vec3(eyeX, eyeY, 0), // and looks at the origin
					glm::vec3(0, (flipped ? -1 : 1), 0) // Head is up (set to 0,-1,0 to look upside-down)
		);
	viewProjectionMatrix = projectionMatrix * viewMatrix;
}

bool Graphics::isFilling() {
	return filling;
}

void Graphics::noFill() {
	filling = false;
}

void Graphics::fill() {
	filling = true;
}

float Graphics::getStrokeWeight() {
	return strokeWeight;
}
void Graphics::setStrokeWeight(float f) {
	if (f != strokeWeight) {
		strokeWeight = f;
		api->setLineWidth(f);
	}
}

void Graphics::setHexColor(int hex, float a) {
	auto c = hexColor(hex);
	c.a	   = a;
	setColor(c);
}
void Graphics::setColor(float bri) {
	setColor(bri, bri, bri, 1);
}
void Graphics::setColor(float r, float g, float b, float a) {
	color = glm::vec4(r, g, b, a);
}

void Graphics::setColor(glm::vec3 c) {
	color = glm::vec4(c.r, c.g, c.b, 1.0);
	;
}

void Graphics::setColor(glm::vec4 c) {
	color = c;
}

void Graphics::setColor(glm::vec4 c, float alpha) {
	color	= c;
	color.a = alpha;
}

const vec4 &Graphics::getColor() const {
	return color;
}

void Graphics::drawPlus(vec2 c, int diameter, int thickness) {
	Drawer d;
	Rectf r;
	r.setFromCentre(c, diameter, thickness);
	d.drawRect(r);
	r.setFromCentre(c, thickness, diameter);
	d.drawRect(r);
	d.createVbo(true)->draw(*this);
}

void Graphics::drawCross(vec2 c, int diameter, int thickness) {
	pushMatrix();
	translate(c);
	rotateZ(M_PI / 4.f);
	drawPlus({0, 0}, diameter, thickness);
	popMatrix();
}

void Graphics::drawChevronLeft(vec2 c, int radius, int thickness) {
	Drawer d;
	d.drawChevronLeft(c, radius, thickness);
	d.createVbo(true)->draw(*this);
}
void Graphics::drawChevronDown(vec2 c, int radius, int thickness) {
	Drawer d;
	d.drawChevronDown(c, radius, thickness);
	d.createVbo(true)->draw(*this);
}

void Graphics::drawChevronUp(vec2 c, int radius, int thickness) {
	Drawer d;
	d.drawChevronUp(c, radius, thickness);
	d.createVbo(true)->draw(*this);
}

void Graphics::drawChevronRight(vec2 c, int radius, int thickness) {
	Drawer d;
	d.drawChevronRight(c, radius, thickness);
	d.createVbo(true)->draw(*this);
}

void Graphics::draw(const Rectf &r) {
	drawRect(r);
}

void Graphics::drawRect(float x, float y, float width, float height) {
	drawRect(Rectf(x, y, width, height));
}

void Graphics::drawRect(const Rectf &r) {
	if (isFilling()) {
		drawVerts({r.tl(), r.tr(), r.br(), r.br(), r.bl(), r.tl()});
	} else {
		drawVerts({r.tl(), r.tr(), r.br(), r.bl(), r.tl()}, Vbo::PrimitiveType::LineStrip);
	}
}

void Graphics::drawTriangle(vec2 a, vec2 b, vec2 c) {
	drawVerts({a, b, c});
}

void Graphics::drawCircle(glm::vec2 c, float r) {
	drawCircle(c.x, c.y, r);
}
void Graphics::drawCircle(float x, float y, float r) {
	static constexpr auto circleResolution = 100;
	auto circleResolationFun			   = [] {
		  std::array<std::pair<float, float>, circleResolution + 1> values {};
		  for (int i = 0; i <= circleResolution; ++i) {
			  float phi = M_PI * 2.f * i / static_cast<float>(circleResolution);
			  values[i] = {std::cos(phi), std::sin(phi)};
		  }
		  return values;
	};
	static const std::array<std::pair<float, float>, circleResolution + 1> sinCos = circleResolationFun();

	if (isFilling()) {
		std::vector<glm::vec2> verts;
		verts.reserve(circleResolution * 2 + 2);

		verts.emplace_back(x, y);
		for (const auto &[cosVal, sinVal]: sinCos) {
			verts.emplace_back(x + cosVal * r, y + sinVal * r);
			verts.emplace_back(verts[0]);
		}
		verts.pop_back();
		drawVerts(verts, Vbo::PrimitiveType::TriangleStrip);
	} else {
		std::vector<glm::vec2> verts;
		verts.reserve(circleResolution + 2);

		for (const auto &[cosVal, sinVal]: sinCos) {
			verts.emplace_back(x + cosVal * r, y + sinVal * r);
		}
		verts.emplace_back(verts.front());
		drawVerts(verts, Vbo::PrimitiveType::LineStrip);
	}
}

void Graphics::drawArc(glm::vec2 c, float r, float startAngle, float endAngle) {
	std::vector<glm::vec2> verts;

	if (startAngle > endAngle) {
		Log::e() << "drawArc doesn't wrap angles for now! Feel like implementing it?";
	}

	if (isFilling()) {
		verts.reserve((endAngle - startAngle) * 2.f / 0.1f + 2.f);

		verts.emplace_back(c);

		// resolution is about 60
		for (float f = startAngle; f < endAngle; f += 0.1) {
			verts.emplace_back(c.x + cos(f) * r, c.y + sin(f) * r);
			verts.emplace_back(c);
		}
		if (startAngle != endAngle) {
			verts.emplace_back(c.x + cos(endAngle) * r, c.y + sin(endAngle) * r);
			verts.emplace_back(c);
		}

		verts.pop_back();
		drawVerts(verts, Vbo::PrimitiveType::TriangleStrip);
	} else {
		verts.reserve((endAngle - startAngle) / 0.1f + 2.f);

		// resolution is about 60
		for (float f = startAngle; f < endAngle; f += 0.1) {
			verts.emplace_back(c.x + cos(f) * r, c.y + sin(f) * r);
		}
		if (startAngle != endAngle) {
			verts.emplace_back(c.x + cos(endAngle) * r, c.y + sin(endAngle) * r);
		}
		drawLineStrip(verts);
	}
}

void Graphics::setBlendMode(BlendMode newBlendMode) {
	blendMode = newBlendMode;
	api->setBlendMode(blendMode);
}

Graphics::BlendMode Graphics::getBlendMode() const {
	return blendMode;
}
void Graphics::drawVerts(const std::vector<glm::vec2> &verts,
						 const std::vector<glm::vec4> &cols,
						 Vbo::PrimitiveType type) {
	api->drawVerts(verts, cols, type);
}

void Graphics::drawVerts(const std::vector<glm::vec2> &verts, Vbo::PrimitiveType type) {
	api->drawVerts(verts, type);
}

void Graphics::maskOn(const Rectf &r) {
	api->maskOn(r);
}

void Graphics::maskOff() {
	api->maskOff();
}

bool Graphics::isMaskOn() {
	return api->isMaskOn();
}

Rectf Graphics::getMaskRect() {
	return api->getMaskRect();
}

void Graphics::drawLine(glm::vec2 a, glm::vec2 b) {
	drawVerts({a, b}, Vbo::PrimitiveType::Lines);
}

void Graphics::drawLine(float x1, float y1, float x2, float y2) {
	drawLine(glm::vec2(x1, y1), glm::vec2(x2, y2));
}

void Graphics::drawLineStrip(const std::vector<vec2> &pts) {
	drawVerts(pts, Vbo::PrimitiveType::LineStrip);
}

void Graphics::drawRoundedRect(const Rectf &r, float radius) {
	if (radius < 2) drawRect(r);
	auto m = makeRoundedRectVbo(r, radius, isFilling());
	m->draw(*this);
}

void Graphics::draw(const Rectf &r, float radius) {
	if (radius < 2) drawRect(r);
	drawRoundedRect(r, radius);
}

void Graphics::drawRoundedRectShadow(Rectf r, float radius, float shadow) {
	std::vector<glm::vec2> v;
	getPerfectRoundedRectVerts(r, radius, v);

	v.pop_back();
	MitredLine lineDrawer;
	lineDrawer.outside	 = true;
	lineDrawer.thickness = shadow;
	Geometry geom;
	int numVerts = lineDrawer.getVerts(v, geom.verts, geom.indices, MitredLine::OpenOrClosed::Closed);
	geom.cols.reserve(geom.cols.size() + numVerts);

	auto c = color;
	c.a	   = 0.f;
	for (int i = 0; i < numVerts; i += 2) {
		geom.cols.push_back(color);
		geom.cols.push_back(c);
	}

	VboRef m = Vbo::create();
	m->setGeometry(geom);
	m->draw(*this);
}

#include "DefaultFont.inc.h"

std::vector<unsigned char> Graphics::getDefaultFontTTFData() {
	return getDefaultFontData();
}
Font &Graphics::getFont() {
	if (font == nullptr) {
		font = new Font();
		font->load(*this, getDefaultFontData(), 42);
	}
	return *font;
}

void Graphics::deallocateFont() {
	if (font != nullptr) {
		delete font;
		font = nullptr;
	}
}
void Graphics::reloadFont() {
	if (font == nullptr) {
		getFont();
		return;
	}
	font->load(*this, getDefaultFontData(), 42);
}

void Graphics::warpMaskForScissor(Rectf &a) {
	// currently does nothing
}

#include "Triangulator.h"
void Graphics::drawShape(const std::vector<vec2> &shape) {
	if (shape.empty()) return;
	if (isFilling()) {
		Triangulator t;
		std::vector<std::vector<vec2>> verts = {shape};
		std::vector<vec2> outs;
		std::vector<unsigned int> indices;
		t.triangulate(verts, outs, indices);
		VboRef vbo = Vbo::create();
		vbo->setVertices(outs);
		vbo->setIndices(indices);
		vbo->draw(*this, Vbo::PrimitiveType::Triangles);
	} else {
		auto verts = shape;
		verts.push_back(verts[0]);
		drawVerts(verts, Vbo::PrimitiveType::LineStrip);
	}
}
void Graphics::drawText(const std::string &s, float x, float y) {
	getFont().draw(*this, s, x, y);
}

void Graphics::drawTextWithBG(const std::string &s, vec4 bgColor, float x, float y) {
	auto r = getFont().getRect(s, x, y);
	r.inset(-4);
	auto old = getColor();
	setColor(bgColor);
	fill();
	draw(r);
	setColor(old);
	getFont().draw(*this, s, x, y);
}

void Graphics::drawText(const std::string &s, vec2 p) {
	getFont().draw(*this, s, p.x, p.y);
}

void Graphics::drawTextCentred(const std::string &s, glm::vec2 c) {
	getFont().drawCentred(*this, s, c);
}

void Graphics::drawTextVerticallyCentred(const std::string &text, glm::vec2 c) {
	getFont().drawVerticallyCentred(*this, text, c);
}

void Graphics::drawTextHorizontallyCentred(const std::string &text, glm::vec2 c) {
	getFont().drawHorizontallyCentred(*this, text, c);
}

void Graphics::saveScreen(std::string pngPath) {
	ImageRef img = Image::create(width, height, 4);

	api->readScreenPixels(img->data, Rectf(0, 0, width, height));
	img->flipVertical();

	// for some reason on OSX there is a bit of alpha - this makes it fully opaque
	for (int i = 0; i < img->data.size(); i += 4) {
		img->data[i + 3] = 255;
	}
	img->save(pngPath);
}

void Graphics::clear(vec4 bgColor) {
	api->clear(bgColor);
}

void Graphics::clear(vec3 bgColor) {
	clear({bgColor.r, bgColor.g, bgColor.b, 1.0});
}

void Graphics::clear(float c) {
	clear(c, c, c);
}
void Graphics::clear(float r, float g, float b, float a) {
	clear({r, g, b, a});
}
#ifdef MZGL_SOKOL_METAL
#	include "SokolAPI.h"
#else
#	include "OpenGLAPI.h"
#endif

Graphics::Graphics() {
#ifdef MZGL_SOKOL_METAL
	api = std::make_unique<SokolAPI>(*this);
#else
	api = std::make_unique<OpenGLAPI>(*this);
#endif
}

int32_t Graphics::getDefaultFrameBufferId() {
#ifndef MZGL_SOKOL_METAL
	if (auto *openGLApi = dynamic_cast<OpenGLAPI *>(api.get())) {
		return openGLApi->getDefaultFrameBufferId();
	}
#endif
	Log::e() << "getDefaultFrameBufferId only works with OpenGL";
	return 0;
}
