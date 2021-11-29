//
//  SvgVbo.h
//  samploid
//
//  Created by Marek Bereza on 07/03/2018.
//  Copyright © 2018 Marek Bereza. All rights reserved.
//

#pragma once
#include "Vbo.h"
#include <string>
#include <memory>
#include "SVG.h"
#include "Geometry.h"




class SvgVbo;
typedef std::shared_ptr<SvgVbo> SvgVboRef;

class SvgVbo {
public:
	
	static SvgVboRef fromSvgString(const std::string &data, float scale, bool ignoreColor) {
		return SvgVboRef(new SvgVbo(data, true, scale, ignoreColor));
	}
	static SvgVboRef create(std::string path, float scale, bool ignoreColor) {
		return SvgVboRef(new SvgVbo(path, scale, ignoreColor));
	}
	static SvgVboRef create(SVGDoc &d) {
		return SvgVboRef(new SvgVbo(d));
	}
	
	void draw(Graphics &g, float x = 0, float y = 0);
	void drawCentred(Graphics &g, glm::vec2 p) {
		draw(g, p.x - width / 2, p.y - height / 2);
	}
	float width = 0;
	float height = 0;
	
	
private:
	SvgVbo(std::string path, float scale, bool ignoreColor);
	SvgVbo(const std::string &svgString, bool mustBeTrue, float scale, bool ignoreColor);
	SvgVbo(SVGDoc &d); // d should be const
	VboRef vbo;
	void loadFromSvg(SVGDoc &d, bool ignoreColor = false); // d should be const
};

class SvgTriangles;
typedef std::shared_ptr<SvgTriangles> SvgTrianglesRef;


class SvgTriangles {
public:
	static SvgTrianglesRef create(std::string path) {
		return SvgTrianglesRef(new SvgTriangles(path));
	}
	void getTriangles(std::vector<glm::vec2> &verts, std::vector<glm::vec2> &indices);
	float width = 0;
	float height = 0;
	
	
	Geometry geometry;
	
private:
	std::vector<glm::vec2> verts;
	std::vector<glm::vec4> cols;
	std::vector<unsigned int> indices;
	SvgTriangles(std::string path);
};
