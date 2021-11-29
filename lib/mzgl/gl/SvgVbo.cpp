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


void SvgVbo::draw(Graphics &g, float x, float y) {
	if(vbo==nullptr) {
		Log::e() << "SvgVbo: vbo is null";
	}
	bool mustPop = false;
	if(x!=0 || y!=0) {
		g.pushMatrix();
		g.translate(x,y);
		mustPop = true;
	}
	vbo->draw(g);
	
	if(mustPop) {
		g.popMatrix();
	}
}
SvgVbo::SvgVbo(SVGDoc &d) {
	loadFromSvg(d);
}


void SvgVbo::loadFromSvg(SVGDoc &svg, bool ignoreColor) {

    vbo = Vbo::create();
	//vbo.clear();
	width = svg.width;
	height = svg.height;
	
	Geometry geometry;
	
	svg.getTriangles(geometry.verts, geometry.cols, geometry.indices);

	vbo->setVertices(geometry.verts);
	if(!ignoreColor) vbo->setColors(geometry.cols);
	vbo->setIndices(geometry.indices);
}
SvgVbo::SvgVbo(const string &svgString, bool mustBeTrue, float scale, bool ignoreColor) {
	SVGDoc svg;
	if(svg.loadFromString(svgString)) {
		svg.scale(2*scale);
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
	if(svg.load(p)) {
		svg.scale(2*scale);
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
	if(svg.load(p)) {
		svg.scale(2);
		width = svg.width;
		height = svg.height;
		svg.getTriangles(geometry.verts, geometry.cols, geometry.indices);
	} else {
		Log::e() << "ERROR: couldn't load SVG file " << p;
	}
}


