//
//  SvgVbo.h
//  samploid
//
//  Created by Marek Bereza on 07/03/2018.
//  Copyright © 2018 Marek Bereza. All rights reserved.
//

#pragma once
#include <mzgl/gl/Vbo.h>
#include <string>
#include <memory>
#include <mzgl/geom/SVG.h>
#include <mzgl/gl/Geometry.h>

class SvgVbo;
typedef std::shared_ptr<SvgVbo> SvgVboRef;

/**
 * I want to deprecate the SvgVbo object, and just have vbo's, and use the createVbo...()
 * functions here, its a bit cleaner.
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
	static SvgVboRef create(SVGDoc &d, bool ignoreColor = false) { return SvgVboRef(new SvgVbo(d, ignoreColor)); }
	static VboRef createVboFromStringWithMaxDim(const std::string &svgCode, float maxDim, bool ignoreColor);

	static VboRef createVboWithMaxDim(const std::string &path, float maxDim, bool ignoreColor);

	void draw(Graphics &g, float x = 0, float y = 0);
	void drawCentred(Graphics &g, glm::vec2 p) { draw(g, p.x - width / 2, p.y - height / 2); }
	float width	 = 0;
	float height = 0;

	VboRef getVbo() { return vbo; }

private:
	SvgVbo(std::string path, float scale, bool ignoreColor);
	SvgVbo(const std::string &svgString, bool mustBeTrue, float scale, bool ignoreColor);
	SvgVbo(SVGDoc &d, bool ignoreColor = false); // d should be const
	VboRef vbo;
	void loadFromSvg(SVGDoc &d, bool ignoreColor = false); // d should be const
};

class SvgTriangles;
typedef std::shared_ptr<SvgTriangles> SvgTrianglesRef;

class SvgTriangles {
public:
	static SvgTrianglesRef create(std::string path) { return SvgTrianglesRef(new SvgTriangles(path)); }
	void getTriangles(std::vector<glm::vec2> &verts, std::vector<glm::vec2> &indices);
	float width	 = 0;
	float height = 0;

	Geometry geometry;

private:
	std::vector<glm::vec2> verts;
	std::vector<glm::vec4> cols;
	std::vector<unsigned int> indices;
	SvgTriangles(std::string path);
};
