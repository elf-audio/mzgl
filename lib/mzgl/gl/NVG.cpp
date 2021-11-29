//
//  NVG.cpp
//  mzgl
//
//  Created by Marek Bereza on 18/05/2020.
//  Copyright © 2020 Marek Bereza. All rights reserved.
//
#include "Graphics.h"
#include "log.h"
#include "NVG.h"

#include "nanovg.h"
//#ifdef MZGL_GL2
//#include <OpenGL/gl.h>
//#else
//#include <OpenGL/gl3.h>
//#endif

#include "mzOpenGL.h"

#ifdef MZGL_GL2
#define NANOVG_GL2_IMPLEMENTATION
#else
#define NANOVG_GL3_IMPLEMENTATION
#endif
#include "nanovg_gl.h"

#include <glm/gtc/type_ptr.hpp>
using namespace std;

void NVG::init() {
	if(inited) return;
	//	#ifdef DEMO_MSAA
#ifdef NANOVG_GL3_IMPLEMENTATION
	vg = nvgCreateGL3(NVG_STENCIL_STROKES | NVG_DEBUG);
#else
	vg = nvgCreateGL2(NVG_STENCIL_STROKES | NVG_DEBUG);
#endif
	//	#else
	//		vg = nvgCreateGL2(NVG_ANTIALIAS | NVG_STENCIL_STROKES | NVG_DEBUG);
	//	#endif
	nvgSetTranformFunction(vg,
							   [this](float *dx, float *dy, float sx, float sy) {
			const auto &m = graphics->getModelMatrix();
			
			*dx = sx*m[0][0] + sy * m[1][0] + m[3][0];
			*dy = sx*m[0][1] + sy * m[1][1] + m[3][1];
		});
	nvgSetTranformMultFunction(vg, [this](float *m3x3) {
		// so here, I've got to multiply this matrix by our model matrix
		// except we need to take one dimension out of the model - you just
		// take out the second last column to do that.
		//   [a c e]
		//   [b d f]
		//   [0 0 1]
		const auto *s = glm::value_ptr(graphics->getModelMatrix());
		
		float t0 = m3x3[0] * s[0] + m3x3[1] * s[4];
		float t2 = m3x3[2] * s[0] + m3x3[3] * s[4];
		float t4 = m3x3[4] * s[0] + m3x3[5] * s[4] + s[12];
		m3x3[1] = m3x3[0] * s[1] + m3x3[1] * s[5];
		m3x3[3] = m3x3[2] * s[1] + m3x3[3] * s[5];
		m3x3[5] = m3x3[4] * s[1] + m3x3[5] * s[5] + s[13];
		m3x3[0] = t0;
		m3x3[2] = t2;
		m3x3[4] = t4;
		
		
		/*
		 float t0 = m3x3[0] * s[0] + m3x3[1] * s[2];
		 float t2 = m3x3[2] * s[0] + m3x3[3] * s[2];
		 float t4 = m3x3[4] * s[0] + m3x3[5] * s[2] + s[4];
		 m3x3[1] = m3x3[0] * s[1] + m3x3[1] * s[3];
		 m3x3[3] = m3x3[2] * s[1] + m3x3[3] * s[3];
		 m3x3[5] = m3x3[4] * s[1] + m3x3[5] * s[3] + s[5];
		 m3x3[0] = t0;
		 m3x3[2] = t2;
		 m3x3[4] = t4;
		 */
	});


		if (vg == NULL) {
			printf("Could not init nanovg.\n");
			return;
		}
	inited = true;
}
void NVG::begin(Graphics &g) {
	
	init();
	
	graphics = &g;
	nvgBeginFrame(vg, g.width, g.height, g.pixelScale);
}


void NVG::end() {
	nvgEndFrame(vg);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

NVG::~NVG() {
	
	#ifdef NANOVG_GL3_IMPLEMENTATION
		nvgDeleteGL3(vg);
	#else
		nvgDeleteGL2(vg);
	#endif
	
}

void NVG::drawRoundedRect(const Rectf &r, float radius) {
	nvgBeginPath(vg);
	// Creates new rounded rectangle shaped sub-path.
	nvgRoundedRect(vg, r.x, r.y, r.width, r.height, radius);
	stylePath();
}


void NVG::drawRoundedRect(const Rectf &r, float tlr, float trr, float brr, float blr) {
	nvgBeginPath(vg);
	// Creates new rounded rectangle shaped sub-path with varying radii for each corner.
	nvgRoundedRectVarying(vg, r.x, r.y, r.width, r.height, tlr, trr, brr, blr);
	stylePath();
}


void NVG::drawEllipse(const Rectf &r) {
	nvgBeginPath(vg);
	vec2 c = r.centre();
	nvgEllipse(vg, c.x, c.y, r.width, r.height);
	stylePath();
}

void NVG::drawCircle(vec2 c, float r) {
	nvgBeginPath(vg);
	nvgCircle(vg, c.x, c.y, r);
	stylePath();
}



void NVG::drawLine(vec2 a, vec2 b) {
	nvgBeginPath(vg);
	nvgMoveTo(vg, a.x, a.y);
	nvgLineTo(vg, b.x, b.y);
	stylePath();
}

void NVG::setDefaultFont(string path, float size) {
	init();
	defaultFontSize = size;
	printf("Loading font from %s\n", path.c_str());
	defaultFont = nvgCreateFont(vg, "default", path.c_str());
}

void NVG::prepareDefaultFont() {
	if(defaultFont==-1) {
		vector<unsigned char> fontData = graphics->getDefaultFontTTFData();
		defaultFont = nvgCreateFontMem(vg, "default", fontData.data(), (int)fontData.size(), 0);
	}
	nvgFontFaceId(vg, defaultFont);
	nvgFontSize(vg, defaultFontSize);
}

void NVG::drawText(string text, vec2 pos, int alignHoriz, int alignVert) {
	prepareDefaultFont();
	int alignment = 0;
	switch(alignHoriz) {
		case -1: alignment = NVG_ALIGN_LEFT; break;
		case 0: alignment = NVG_ALIGN_CENTER; break;
		case 1: alignment = NVG_ALIGN_RIGHT; break;
		default: Log::e() << "invalid horizontal alignment in NVG::drawText()"; break;
	}
	
	switch(alignVert) {
		case -1: alignment |= NVG_ALIGN_TOP; break;
		case 0: alignment  |= NVG_ALIGN_MIDDLE; break;
		case 1: alignment  |= NVG_ALIGN_BOTTOM; break;
		default: Log::e() << "invalid vertical alignment in NVG::drawText()"; break;
	}
	
	nvgTextAlign(vg, alignment);
	nvgText(vg, pos.x, pos.y, text.c_str(), NULL);
	
}
void NVG::drawText(string text, vec2 pos) {
	prepareDefaultFont();
	
	/*
	 
	 
	 NVG_ALIGN_LEFT 		= 1<<0,	// Default, align text horizontally to left.
	 NVG_ALIGN_CENTER 	= 1<<1,	// Align text horizontally to center.
	 NVG_ALIGN_RIGHT 	= 1<<2,	// Align text horizontally to right.
	 // Vertical align
	 NVG_ALIGN_TOP 		= 1<<3,	// Align text vertically to top.
	 NVG_ALIGN_MIDDLE	= 1<<4,	// Align text vertically to middle.
	 NVG_ALIGN_BOTTOM	= 1<<5,	// Align text vertically to bottom.
	 NVG_ALIGN_BASELINE	= 1<<6, // Default, align text vertically to baseline.
	 
	 
	 
	 */
	nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_BASELINE);
	nvgText(vg, pos.x, pos.y, text.c_str(), NULL);
}

void NVG::drawTextCentred(string text, vec2 pos) {
	
	prepareDefaultFont();
	nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
	nvgText(vg, pos.x, pos.y, text.c_str(), NULL);

}


void NVG::drawRect(const Rectf &r) {
	nvgBeginPath(vg);
	nvgRect(vg, r.x, r.y, r.width, r.height);
	stylePath();
	
}

void NVG::drawTriangle(vec2 a, vec2 b, vec2 c) {
	nvgBeginPath(vg);
	nvgMoveTo(vg, a.x, a.y);
	nvgLineTo(vg, b.x, b.y);
	nvgLineTo(vg, c.x, c.y);
	stylePath();
	
}
void NVG::stylePath() {
	
	if(filling) {
		nvgFill(vg);
	}
	
	if(stroking) {
		nvgStroke(vg);
	}
}



void NVG::strokeWidth(float f) {
	nvgStrokeWidth(vg, f);
}
void NVG::mitreLimit(float limit) {
	nvgMiterLimit(vg, limit);
}
void NVG::roundCaps() {
	nvgLineCap(vg, NVG_ROUND);
}

void NVG::buttCaps() {
	nvgLineCap(vg, NVG_BUTT);
}

void NVG::squareCaps() {
	nvgLineCap(vg, NVG_SQUARE);
}

void NVG::roundJoins() {
	nvgLineJoin(vg, NVG_ROUND);
}
void NVG::buttJoins() {
	nvgLineJoin(vg, NVG_BUTT);
}
void NVG::squareJoins() {
	nvgLineJoin(vg, NVG_SQUARE);
}

void NVG::stroke() {
	nvgStrokeColor(vg, nvgRGBAf(strokeColor.r, strokeColor.g, strokeColor.b, strokeColor.a));
	stroking = true;
}

void NVG::stroke(vec4 c) {
	strokeColor = c;
	
	stroke();
}
void NVG::stroke(float bri) {
	strokeColor = {bri, bri, bri, 1.0};
	stroke();
}
void NVG::noStroke() {
	stroking = false;
}


void NVG::strokeGradientLinear(vec4 colA, vec4 colB, vec2 posA, vec2 posB) {
	nvgStrokePaint(vg, nvgLinearGradient(vg, posA.x, posA.y, posB.x, posB.y, nvgRGBAf(colA.r, colA.g, colA.b, colA.a),
	nvgRGBAf(colB.r, colB.g, colB.b, colB.a)));
	stroking = true;
}

void NVG::strokeGradientRadial(vec4 colA, vec4 colB, vec2 centre, float innerRadius, float outerRadius) {
	nvgStrokePaint(vg, nvgRadialGradient(vg, centre.x, centre.y, innerRadius, outerRadius,
	  nvgRGBAf(colA.r, colA.g, colA.b, colA.a),
	nvgRGBAf(colB.r, colB.g, colB.b, colB.a)));
	stroking = true;
}
void NVG::strokeGradientBox(vec4 colA, vec4 colB, Rectf rect, float radius, float feather) {
	nvgStrokePaint(vg, nvgBoxGradient(vg, rect.x, rect.y, rect.width, rect.height, radius, feather,
	nvgRGBAf(colA.r, colA.g, colA.b, colA.a),
	nvgRGBAf(colB.r, colB.g, colB.b, colB.a)));
	stroking = true;
}



void NVG::fill() {
	nvgFillColor(vg, nvgRGBAf(fillColor.r, fillColor.g, fillColor.b, fillColor.a));
	filling = true;
	
}

void NVG::fill(vec4 c) {
	fillColor = c;
	
	fill();
}
void NVG::fill(float bri) {
	fillColor = {bri, bri, bri, 1.0};
	fill();
}

void NVG::noFill() {
	filling = false;
}


void NVG::fillGradientLinear(vec4 colA, vec4 colB, vec2 posA, vec2 posB) {
	
	nvgFillPaint(vg, nvgLinearGradient(vg, posA.x, posA.y, posB.x, posB.y, nvgRGBAf(colA.r, colA.g, colA.b, colA.a),
								  nvgRGBAf(colB.r, colB.g, colB.b, colB.a)));
	filling = true;
}

void NVG::fillGradientRadial(vec4 colA, vec4 colB, vec2 centre, float innerRadius, float outerRadius) {
	
	nvgFillPaint(vg, nvgRadialGradient(vg, centre.x, centre.y, innerRadius, outerRadius,
								  nvgRGBAf(colA.r, colA.g, colA.b, colA.a),
								nvgRGBAf(colB.r, colB.g, colB.b, colB.a)));
	filling = true;
}

void NVG::fillGradientBox(vec4 colA, vec4 colB, Rectf rect, float radius, float feather) {
	
	nvgFillPaint(vg, nvgBoxGradient(vg, rect.x, rect.y, rect.width, rect.height, radius, feather,
							   nvgRGBAf(colA.r, colA.g, colA.b, colA.a),
							   nvgRGBAf(colB.r, colB.g, colB.b, colB.a)));
	filling = true;
}


void NVG::beginScissor(const Rectf &r) {
	nvgScissor(vg, r.x, r.y, r.width, r.height);
}

void NVG::endScissor() {
	nvgResetScissor(vg);
}
void NVG::drawLineStrip(const vector<vec2> &v) {
	if(v.size()<2) return;
	nvgBeginPath(vg);
	nvgMoveTo(vg, v[0].x, v[0].y);
	for(int i = 1; i < v.size(); i++) {
		nvgLineTo(vg, v[i].x, v[i].y);
	}
	stylePath();
}
void NVG::drawLines(const vector<vec2> &v) {
	if(v.size()<2) return;
	nvgBeginPath(vg);
	
	for(int i = 1; i+1 < v.size(); i+=2) {
		nvgMoveTo(vg, v[i].x, v[i].y);
		nvgLineTo(vg, v[i+1].x, v[i+1].y);
	}
	stylePath();
}
