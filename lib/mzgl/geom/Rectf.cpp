//
//  Rectf.cpp
//  samploid
//
//  Created by Marek Bereza on 16/02/2018.
//  Copyright © 2018 Marek Bereza. All rights reserved.
//

#include <mzgl/geom/Rectf.h>

void Rectf::inset(float amt) {
	x += amt;
	y += amt;
	width -= amt * 2;
	height -= amt * 2;
}

void Rectf::inset(glm::vec2 amt) {
	inset(amt.x, amt.y);
}

void Rectf::inset(float horiz, float vert) {
	x += horiz;
	width -= horiz * 2;

	y += vert;
	height -= vert * 2;
}

void Rectf::inset(float top, float right, float bottom, float left) {
	x += left;
	y += top;
	width -= left + right;
	height -= top + bottom;
}

void Rectf::set(float x, float y, float w, float h) {
	this->x		 = x;
	this->y		 = y;
	this->width	 = w;
	this->height = h;
}

void Rectf::set(const Rectf &r) {
	this->x		 = r.x;
	this->y		 = r.y;
	this->width	 = r.width;
	this->height = r.height;
}

bool Rectf::inside(glm::vec2 p) const {
	return inside(p.x, p.y);
}

void Rectf::setFromCentre(float cx, float cy, float w, float h) {
	x	   = cx - w / 2.f;
	y	   = cy - h / 2.f;
	width  = w;
	height = h;
}
void Rectf::alignToPixels() {
	x	   = (int) x;
	y	   = (int) y;
	width  = (int) width;
	height = (int) height;
}

bool Rectf::inside(float x, float y) const {
	//	bool insideX = width>0 ? x >= this->x &&
	//	x <= right() : x <= this->x && x >= this->x + this->width;
	//
	//	bool insideY = height>0 ? y >= this->y &&
	//	y <= bottom() : y <= this->y && y >= this->y + this->height;
	//
	//	return insideX && insideY;
	//

	return x >= getMinX() && y >= getMinY() && x <= getMaxX() && y <= getMaxY();
}

bool Rectf::inside(const Rectf &r) const {
	return inside(r.getMinX(), r.getMinY()) && inside(r.getMaxX(), r.getMaxY());
}
bool Rectf::intersects(const Rectf &rect) const {
	return (getMinX() < rect.getMaxX() && getMaxX() > rect.getMinX() && getMinY() < rect.getMaxY()
			&& getMaxY() > rect.getMinY());
}

void Rectf::growToInclude(glm::vec2 p) {
	// taken from oF again
	float x0 = fmin(getMinX(), p.x);
	float x1 = fmax(getMaxX(), p.x);
	float y0 = fmin(getMinY(), p.y);
	float y1 = fmax(getMaxY(), p.y);
	float w	 = x1 - x0;
	float h	 = y1 - y0;
	set(x0, y0, w, h);
}

void Rectf::growToInclude(const Rectf &rect) {
	float x0 = fmin(getMinX(), rect.getMinX());
	float x1 = fmax(getMaxX(), rect.getMaxX());
	float y0 = fmin(getMinY(), rect.getMinY());
	float y1 = fmax(getMaxY(), rect.getMaxY());
	float w	 = x1 - x0;
	float h	 = y1 - y0;
	set(x0, y0, w, h);
}

// again taken from oF
Rectf Rectf::getIntersection(const Rectf &rect) const {
	float x0 = fmax(getMinX(), rect.getMinX());
	float x1 = fmin(getMaxX(), rect.getMaxX());

	float w = x1 - x0;
	if (w < 0.0f) return Rectf(0, 0, 0, 0);

	float y0 = fmax(getMinY(), rect.getMinY());
	float y1 = fmin(getMaxY(), rect.getMaxY());

	float h = y1 - y0;
	if (h < 0.0f) return Rectf(0, 0, 0, 0);

	return Rectf(x0, y0, w, h);
}
