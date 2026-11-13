//
//  SvgVbo.cpp
//  samploid
//
//  Created by Marek Bereza on 07/03/2018.
//  Copyright © 2018 Marek Bereza. All rights reserved.
//

#include "SvgVbo.h"

#include "SvgDocument.h"
#include "util.h"
#include "log.h"

using namespace std;

namespace {
#ifdef __ANDROID__
	string resolvePath(const string &path) {
		return path;
	}
#else
	string resolvePath(const string &path) {
		return dataPath(path);
	}
#endif

	// top left at the origin, s times the document's size
	glm::mat3 scaleBy(float s) {
		return {s, 0, 0, 0, s, 0, 0, 0, 1};
	}
} // namespace

void SvgVbo::draw(Graphics &g, float x, float y) {
	if (vbo == nullptr) {
		Log::e() << "SvgVbo: vbo is null";
		return;
	}

	if (x != 0 || y != 0) {
		ScopedTranslate scp(g, x, y);
		vbo->draw(g);
	} else {
		vbo->draw(g);
	}
}

void SvgVbo::loadFromSvg(const SvgDocument &svg, float scale, bool ignoreColor) {
	const float s = 2 * scale;
	width		  = svg.width * s;
	height		  = svg.height * s;

	SvgDocument::Options options;
	options.tolerance = svg::scaleIndependentTolerance(svg, s, s);
	vbo				  = svg::createVbo(svg, scaleBy(s), ignoreColor, options);
}

SvgVbo::SvgVbo(const string &svgString, bool mustBeTrue, float scale, bool ignoreColor) {
	SvgDocument svg;
	if (svg.parse(svgString)) {
		loadFromSvg(svg, scale, ignoreColor);
	} else {
		Log::e() << "ERROR: couldn't load SVG from string";
	}
}

SvgVbo::SvgVbo(string path, float scale, bool ignoreColor) {
	const string p = resolvePath(path);
	SvgDocument svg;
	if (svg.load(p)) {
		loadFromSvg(svg, scale, ignoreColor);
	} else {
		Log::e() << "ERROR: couldn't load SVG file " << p;
	}
}

SvgTriangles::SvgTriangles(string path) {
	const string p = resolvePath(path);
	SvgDocument svg;
	if (svg.load(p)) {
		const float s = 2;
		width		  = svg.width * s;
		height		  = svg.height * s;
		SvgDocument::Options options;
		options.tolerance = svg::scaleIndependentTolerance(svg, s, s);
		svg.tessellate(scaleBy(s), geometry, options);
	} else {
		Log::e() << "ERROR: couldn't load SVG file " << p;
	}
}
