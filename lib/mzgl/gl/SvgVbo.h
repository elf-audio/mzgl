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

class SvgVbo;
typedef std::shared_ptr<SvgVbo> SvgVboRef;

/**
 * I want to deprecate the SvgVbo object, and just have vbo's, and use the createVbo...()
 * functions here, its a bit cleaner.
 *
 * The createVbo...() functions that take a size (width, height, max dimension)
 * centre the artwork on the origin and tessellate it for that size in pixels.
 */
class SvgVbo {
public:
	static SvgVboRef fromSvgString(const std::string &data, float scale, bool ignoreColor) {
		return SvgVboRef(new SvgVbo(data, true, scale, ignoreColor));
	}

	static VboRef createVboFromStringWithScale(const std::string &svgCode, float scale, bool ignoreColor);
	static VboRef createVboFromStringWithHeight(const std::string &svgCode, float height, bool ignoreColor);
	static VboRef createVboFromStringWithWidth(const std::string &svgCode, float width, bool ignoreColor);
	static VboRef createVboFromStringWithWidthAndHeight(const std::string &svgCode,
														float width,
														float height,
														bool ignoreColor);

	static SvgVboRef create(std::string path, float scale, bool ignoreColor) {
		return SvgVboRef(new SvgVbo(path, scale, ignoreColor));
	}
	static VboRef createVboFromStringWithMaxDim(const std::string &svgCode, float maxDim, bool ignoreColor);

	static VboRef createVboWithMaxDim(const std::string &path, float maxDim, bool ignoreColor);

	// for anything the functions above don't cover: transform maps the document
	// (viewBox origin at 0,0) to where it's drawn, in pixels - build one from
	// SvgDocument::centredWith...()
	static VboRef createVbo(const SvgDocument &doc, const glm::mat3 &transform, bool ignoreColor);

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

class SvgTriangles {
public:
	static SvgTrianglesRef create(std::string path) { return SvgTrianglesRef(new SvgTriangles(path)); }
	float width	 = 0;
	float height = 0;

	Geometry geometry;

private:
	SvgTriangles(std::string path);
};
