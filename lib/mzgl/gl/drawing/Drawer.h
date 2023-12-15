//
//  Drawer.h
//  samploid
//
//  Created by Marek Bereza on 05/03/2018.
//  Copyright © 2018 Marek Bereza. All rights reserved.
//

#pragma once

#include "MitredLine.h"
#include "Graphics.h"
#include "Geometry.h"

class Drawer {
public:
	float strokeWeight = 1; // this should be private
	bool filled		   = true;
	glm::vec4 color;

	void setStrokeWeight(float strokeWeight) { this->strokeWeight = strokeWeight; }
	Geometry geom;

	MitredLine lineDrawer;
	void fill();
	void noFill();
	void setColor(float grey);
	void setColor(float r, float g, float b, float a = 1.f);
	void setColor(const glm::vec4 &c);
	void setColor(const glm::vec4 &c, float alpha);

	void calculateNormalizedTexCoords() { geom.calculateNormalizedTexCoords(); }
	// only working for rects an rounded rects at the moment, and also only vertical
	// and horizontal gradients work really nicely - for a better diagonal gradient
	// you'd need a colour gradient matrix sent to the shader to transform all the colours
	void setGradient(glm::vec4 c1, glm::vec2 pos1, glm::vec4 c2, glm::vec2 pos2);

	void drawTriangle(glm::vec2 a, glm::vec2 b, glm::vec2 c);
	// triangle with colours
	void drawTriangle(glm::vec2 a, glm::vec2 b, glm::vec2 c, glm::vec4 ca, glm::vec4 cb, glm::vec4 cc);

	void drawQuad(glm::vec2 a, glm::vec2 b, glm::vec2 c, glm::vec2 d);

	void drawLine(glm::vec2 a, glm::vec2 b);
	void drawLine(float ax, float ay, float bx, float by);
	void drawLineStrip(const std::vector<vec2> &strip);
	void drawLineStrip(const std::vector<vec2> &strip, const std::vector<vec4> &cols);
	void drawBevelledLineStrip(const std::vector<vec2> &strip);

	void drawTriangleStrip(const std::vector<vec2> &strip);
	void drawTriangleFan(const std::vector<vec2> &fan);
	void drawTriangles(const std::vector<vec2> &verts, const std::vector<uint32_t> &indices);
	void drawRect(const Rectf &r);
	void drawRect(float x, float y, float width, float height);

	void drawCircle(glm::vec2 c, float r);

	void drawArc(glm::vec2 c, float r, float startAngle, float endAngle);

	void drawRoundedRect(const Rectf &r, float radius);
	void drawRoundedRectShadow(const Rectf &r, float radius, float shadow);
	void drawRoundedRect(const Rectf &r, float radius, bool tl, bool tr, bool br, bool bl);
	void drawPlus(vec2 c, int diameter, int thickness);
	void drawCross(vec2 c, int diameter, int thickness);
	void drawChevronUp(vec2 c, int radius, int thickness);
	void drawChevronDown(vec2 c, int radius, int thickness);
	void drawChevronLeft(vec2 c, int radius, int thickness);
	void drawChevronRight(vec2 c, int radius, int thickness);
	void commit(VboRef vbo, bool ignoreColor = false, bool addNormalizedTexCoords = false);
	VboRef createVbo(bool ignoreColor = false, bool addNormalizedTexCoords = false);
	void addGeometry(Geometry &_geom);
	bool isEmpty();

private:
	void getPerfectRoundedRectVerts(
		const Rectf &r, float radius, std::vector<glm::vec2> &outVerts, bool tl, bool tr, bool br, bool bl);
	void roundedRectVerts(const Rectf &r,
						  float radius,
						  std::vector<glm::vec2> &outVerts,
						  std::vector<glm::vec2> &cache,
						  bool tl,
						  bool tr,
						  bool br,
						  bool bl);

	void createRoundedRectCache(std::vector<glm::vec2> &cache, int numSteps);
	bool isDoingGradient = false;
	vec4 lookupGradient(vec2 pos);

	vec4 gradientStartColor;
	vec4 gradientStopColor;
	vec2 gradientStartPoint;
	vec2 gradientStopPoint;

	vec2 v2;
	float v2_ls;

	// rounded rect vector so we don't have to keep reallocating
	std::vector<glm::vec2> rrv;
};

/*
 

 
 TODO To speed it up
 
 1 Drawer.drawRoundedRect is slow mostly because of Mitred line (mostly getStripCoord)
 roundedRectVerts could preallocate space in the array
 
 */
