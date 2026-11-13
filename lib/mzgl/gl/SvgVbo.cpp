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
	// The functions that take a size in pixels tessellate for that size. The
	// ones that take a bare scale factor don't say how big the result ends up
	// on screen - callers go on to g.scale() it or scale the geometry - so
	// those get a tolerance relative to the artwork instead: as smooth as
	// SvgDocument's default at 500 px across, whatever size it's built at.
	float scaleIndependentTolerance(const SvgDocument &doc, float sx, float sy) {
		const float maxDim = std::max(std::abs(doc.width * sx), std::abs(doc.height * sy));
		return std::max(maxDim / 2000.f, 1e-4f);
	}

	VboRef vboFromGeometry(const Geometry &geometry) {
		auto vbo = Vbo::create();
		vbo->setGeometry(geometry);
		return vbo;
	}

	VboRef vboFromString(const std::string &svgCode,
						 bool ignoreColor,
						 const std::function<glm::mat3(const SvgDocument &)> &transform) {
		SvgDocument doc;
		if (!doc.parse(svgCode) || doc.width <= 0 || doc.height <= 0) {
			return nullptr;
		}
		return SvgVbo::createVbo(doc, transform(doc), ignoreColor);
	}

#ifdef __ANDROID__
	string resolvePath(const string &path) {
		return path;
	}
#else
	string resolvePath(const string &path) {
		return dataPath(path);
	}
#endif
} // namespace

VboRef SvgVbo::createVbo(const SvgDocument &doc, const glm::mat3 &transform, bool ignoreColor) {
	SvgDocument::Options options;
	options.colors = !ignoreColor;
	Geometry geometry;
	doc.tessellate(transform, geometry, options);
	return vboFromGeometry(geometry);
}

VboRef SvgVbo::createVboWithMaxDim(const std::string &path, float maxDim, bool ignoreColor) {
	SvgDocument doc;
	if (!doc.load(path) || doc.width <= 0 || doc.height <= 0) {
		return nullptr;
	}
	return createVbo(doc, doc.centredWithMaxDim(maxDim), ignoreColor);
}

VboRef SvgVbo::createVboFromStringWithMaxDim(const std::string &svgCode, float maxDim, bool ignoreColor) {
	return vboFromString(svgCode, ignoreColor, [&](const SvgDocument &d) { return d.centredWithMaxDim(maxDim); });
}

VboRef SvgVbo::createVboFromStringWithHeight(const std::string &svgCode, float height, bool ignoreColor) {
	return vboFromString(svgCode, ignoreColor, [&](const SvgDocument &d) { return d.centredWithHeight(height); });
}

VboRef SvgVbo::createVboFromStringWithWidth(const std::string &svgCode, float width, bool ignoreColor) {
	return vboFromString(svgCode, ignoreColor, [&](const SvgDocument &d) { return d.centredWithWidth(width); });
}

VboRef SvgVbo::createVboFromStringWithScale(const std::string &svgCode, float scale, bool ignoreColor) {
	SvgDocument doc;
	if (!doc.parse(svgCode) || doc.width <= 0 || doc.height <= 0) {
		return nullptr;
	}
	SvgDocument::Options options;
	options.colors	  = !ignoreColor;
	options.tolerance = scaleIndependentTolerance(doc, scale, scale);
	Geometry geometry;
	doc.tessellate(doc.centredWithScale(scale), geometry, options);
	return vboFromGeometry(geometry);
}

VboRef SvgVbo::createVboFromStringWithWidthAndHeight(const std::string &svgCode,
													 float width,
													 float height,
													 bool ignoreColor) {
	return vboFromString(svgCode, ignoreColor, [&](const SvgDocument &d) {
		return d.centredWithScale(width / d.width, height / d.height);
	});
}

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

// top left at the origin, 2 * scale times the document's size
void SvgVbo::loadFromSvg(const SvgDocument &svg, float scale, bool ignoreColor) {
	const float s = 2 * scale;
	width		  = svg.width * s;
	height		  = svg.height * s;

	SvgDocument::Options options;
	options.colors	  = !ignoreColor;
	options.tolerance = scaleIndependentTolerance(svg, s, s);
	Geometry geometry;
	svg.tessellate(glm::mat3 {s, 0, 0, 0, s, 0, 0, 0, 1}, geometry, options);
	vbo = vboFromGeometry(geometry);
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
		options.tolerance = scaleIndependentTolerance(svg, s, s);
		svg.tessellate(glm::mat3 {s, 0, 0, 0, s, 0, 0, 0, 1}, geometry, options);
	} else {
		Log::e() << "ERROR: couldn't load SVG file " << p;
	}
}
