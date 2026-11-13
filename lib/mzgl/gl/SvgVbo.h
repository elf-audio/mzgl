//
//  SvgVbo.h
//  samploid
//
//  Created by Marek Bereza on 07/03/2018.
//  Copyright © 2018 Marek Bereza. All rights reserved.
//

#pragma once
#include "Graphics.h"
#include "Vbo.h"
#include <string>
#include <memory>
#include "SvgDocument.h"
#include "Geometry.h"
#include "svgFactories.h"
#include "DisableAllWarnings.h"

#define MZGL_SVGVBO_DEPRECATED                                                                                    \
	"SvgVbo is built at a scale factor and scaled again when drawn, so it can't be tessellated for its real size. " \
	"Use svg::createVboWith...() (svgFactories.h) with the size in pixels and keep the VboRef."

class [[deprecated(MZGL_SVGVBO_DEPRECATED)]] SvgVbo;
// naming the class here would warn in every file that includes this header
DISABLE_WARNINGS
using SvgVboRef [[deprecated(MZGL_SVGVBO_DEPRECATED)]] = std::shared_ptr<SvgVbo>;
RESTORE_WARNINGS

/**
 * DEPRECATED - a vbo of an svg plus its size, built at `scale` (times 2) with
 * its top left at the origin. The functions that used to live here as statics
 * are in svgFactories.h (namespace svg) and are what new code should use.
 */
class [[deprecated(MZGL_SVGVBO_DEPRECATED)]] SvgVbo {
public:
	static std::shared_ptr<SvgVbo> fromSvgString(const std::string &data, float scale, bool ignoreColor) {
		return std::shared_ptr<SvgVbo>(new SvgVbo(data, true, scale, ignoreColor));
	}

	static std::shared_ptr<SvgVbo> create(std::string path, float scale, bool ignoreColor) {
		return std::shared_ptr<SvgVbo>(new SvgVbo(path, scale, ignoreColor));
	}

	void draw(Graphics &g, float x = 0, float y = 0);
	void drawCentred(Graphics &g, glm::vec2 p) { draw(g, p.x - width / 2, p.y - height / 2); }
	float width	 = 0;
	float height = 0;

	VboRef getVbo() { return vbo; }

private:
	SvgVbo(std::string path, float scale, bool ignoreColor);
	SvgVbo(const std::string &svgString, bool mustBeTrue, float scale, bool ignoreColor);
	VboRef vbo;
	void loadFromSvg(const SvgDocument &d, float scale, bool ignoreColor);
};

class SvgTriangles;
typedef std::shared_ptr<SvgTriangles> SvgTrianglesRef;

/**
 * The triangles of an svg at twice its document size, top left at the origin,
 * for callers that scale and recolour the geometry themselves. Same caveat as
 * SvgVbo: it doesn't know the final size. SvgDocument::tessellate() straight
 * into a Drawer's geometry does the same job at the right size.
 */
class SvgTriangles {
public:
	static SvgTrianglesRef create(std::string path) { return SvgTrianglesRef(new SvgTriangles(path)); }
	float width	 = 0;
	float height = 0;

	Geometry geometry;

private:
	SvgTriangles(std::string path);
};
