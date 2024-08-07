//
//  SvgVbo.cpp
//  samploid
//
//  Created by Marek Bereza on 07/03/2018.
//  Copyright © 2018 Marek Bereza. All rights reserved.
//

#include "SvgVbo.h"

#include "SVG.h"
#include "util.h"
#include "log.h"

using namespace std;

static VboRef vboFromDoc(SVGDoc &doc, float maxDim, bool ignoreColor) {
	vec2 tr = -vec2(doc.width / 2, doc.height / 2);
	doc.translate(tr.x, tr.y);

	float actualMaxDim = std::max(doc.width, doc.height);

	doc.scale(maxDim / actualMaxDim);
	return SvgVbo::create(doc, ignoreColor)->getVbo();
}

static VboRef vboFromDocWithHeight(SVGDoc &doc, float height, bool ignoreColor) {
	vec2 tr = -vec2(doc.width / 2, doc.height / 2);
	doc.translate(tr.x, tr.y);

	doc.scale(height / doc.height);
	return SvgVbo::create(doc, ignoreColor)->getVbo();
}

static VboRef vboFromDocWithWidth(SVGDoc &doc, float width, bool ignoreColor) {
	vec2 tr = -vec2(doc.width / 2, doc.height / 2);
	doc.translate(tr.x, tr.y);

	doc.scale(width / doc.width);
	return SvgVbo::create(doc, ignoreColor)->getVbo();
}

static VboRef vboFromDocWithScale(SVGDoc &doc, float scale, bool ignoreColor) {
	vec2 tr = -vec2(doc.width / 2, doc.height / 2);
	doc.translate(tr.x, tr.y);
	doc.scale(scale);
	return SvgVbo::create(doc, ignoreColor)->getVbo();
}

static VboRef vboFromDocWithWidthAndHeight(SVGDoc &doc, float width, float height, bool ignoreColor) {
	vec2 tr = -vec2(doc.width / 2, doc.height / 2);
	doc.translate(tr.x, tr.y);
	doc.scale(width / doc.width, height / doc.height);
	return SvgVbo::create(doc, ignoreColor)->getVbo();
}

VboRef SvgVbo::createVboWithMaxDim(const std::string &path, float maxDim, bool ignoreColor) {
	SVGDoc doc;
	if (!doc.load(path)) {
		return nullptr;
	}
	return vboFromDoc(doc, maxDim, ignoreColor);
}

VboRef SvgVbo::createVboFromStringWithMaxDim(const std::string &svgCode, float maxDim, bool ignoreColor) {
	SVGDoc doc;
	if (!doc.loadFromString(svgCode)) {
		return nullptr;
	}
	return vboFromDoc(doc, maxDim, ignoreColor);
}

VboRef SvgVbo::createVboFromStringWithHeight(const std::string &svgCode, float height, bool ignoreColor) {
	SVGDoc doc;
	if (!doc.loadFromString(svgCode)) {
		return nullptr;
	}
	return vboFromDocWithHeight(doc, height, ignoreColor);
}

VboRef SvgVbo::createVboFromStringWithWidth(const std::string &svgCode, float width, bool ignoreColor) {
	SVGDoc doc;
	if (!doc.loadFromString(svgCode)) {
		return nullptr;
	}
	return vboFromDocWithWidth(doc, width, ignoreColor);
}

VboRef SvgVbo::createVboFromStringWithScale(const std::string &svgCode, float scale, bool ignoreColor) {
	SVGDoc doc;
	doc.loadFromString(svgCode);
	return vboFromDocWithScale(doc, scale, ignoreColor);
}

VboRef SvgVbo::createVboFromStringWithWidthAndHeight(const std::string &svgCode,
													 float width,
													 float height,
													 bool ignoreColor) {
	SVGDoc doc;
	doc.loadFromString(svgCode);
	return vboFromDocWithWidthAndHeight(doc, width, height, ignoreColor);
}

void SvgVbo::draw(Graphics &g, float x, float y) {
	if (vbo == nullptr) {
		Log::e() << "SvgVbo: vbo is null";
	}

	if (x != 0 || y != 0) {
		ScopedTranslate scp(g, x, y);
		vbo->draw(g);
	} else {
		vbo->draw(g);
	}
}

SvgVbo::SvgVbo(SVGDoc &d, bool ignoreColor) {
	loadFromSvg(d, ignoreColor);
}

void SvgVbo::loadFromSvg(SVGDoc &svg, bool ignoreColor) {
	vbo = Vbo::create();
	//vbo.clear();
	width  = svg.width;
	height = svg.height;

	Geometry geometry;

	svg.getTriangles(geometry.verts, geometry.cols, geometry.indices);

	vbo->setVertices(geometry.verts);
	if (!ignoreColor) vbo->setColors(geometry.cols);
	vbo->setIndices(geometry.indices);
}
SvgVbo::SvgVbo(const string &svgString, bool mustBeTrue, float scale, bool ignoreColor) {
	SVGDoc svg;
	if (svg.loadFromString(svgString)) {
		svg.scale(2 * scale);
		loadFromSvg(svg, ignoreColor);
	} else {
		Log::e() << "ERROR: couldn't load SVG from string";
	}
}

SvgVbo::SvgVbo(string path, float scale, bool ignoreColor) {
#ifdef __ANDROID__
	string p = path;
#else
	string p = dataPath(path);
#endif

	SVGDoc svg;
	if (svg.load(p)) {
		svg.scale(2 * scale);
		loadFromSvg(svg, ignoreColor);
	} else {
		Log::e() << "ERROR: couldn't load SVG file " << p;
	}
}

SvgTriangles::SvgTriangles(string path) {
#ifdef __ANDROID__
	string p = path;
#else
	string p = dataPath(path);
#endif
	SVGDoc svg;
	if (svg.load(p)) {
		svg.scale(2);
		width  = svg.width;
		height = svg.height;
		svg.getTriangles(geometry.verts, geometry.cols, geometry.indices);
	} else {
		Log::e() << "ERROR: couldn't load SVG file " << p;
	}
}
