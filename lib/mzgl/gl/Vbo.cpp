//
//  Vbo.cpp
//  MZGL
//
//  Created by Marek Bereza on 15/01/2018.
//  Copyright © 2018 Marek Bereza. All rights reserved.
//

#include "mzOpenGL.h"

#include "Vbo.h"
#include "Shader.h"
#include "error.h"
#include "Graphics.h"
#include "log.h"
#include "Geometry.h"

#ifdef MZGL_SOKOL_METAL
#	include "SokolVbo.h"
#else
#	include "OpenGLVbo.h"
#endif

#ifdef __ANDROID__
std::vector<Vbo *> Vbo::vbos;
#endif

VboRef Vbo::create() {
#ifdef MZGL_SOKOL_METAL
	return VboRef(new SokolVbo());
#else
	return VboRef(new OpenGLVbo());
#endif
}

Vbo &Vbo::setMode(PrimitiveType mode) {
	this->mode = mode;
	return *this;
}

Vbo &Vbo::setGeometry(const Geometry &geom) {
	if (!geom.cols.empty()) setColors(geom.cols);
	if (!geom.verts.empty()) setVertices(geom.verts);
	if (!geom.indices.empty()) setIndices(geom.indices);
	if (!geom.texCoords.empty()) setTexCoords(geom.texCoords);
	return *this;
}

#ifdef DO_DRAW_STATS
int Vbo::numDrawnVerts		 = 0;
int Vbo::numDrawCalls		 = 0;
static int currNumDrawnVerts = 0;
static int currNumDrawCalls	 = 0;
#endif

void Vbo::resetDrawStats() {
	numDrawnVerts	  = currNumDrawnVerts;
	numDrawCalls	  = currNumDrawCalls;
	currNumDrawnVerts = 0;
	currNumDrawCalls  = 0;
}

Vbo::Vbo() {
#ifdef __ANDROID__
	vbos.push_back(this);
#endif
}
Vbo::~Vbo() {
#ifdef __ANDROID__
	for (int i = 0; i < vbos.size(); i++) {
		if (vbos[i] == this) {
			vbos.erase(vbos.begin() + i);
			break;
		}
	}
#endif
}

void Vbo::draw(Graphics &g, vec2 offset) {
	ScopedTranslate t(g, offset);
	draw(g);
}

void Vbo::draw(Graphics &g, PrimitiveType _mode, size_t instances) {
#ifdef DO_DRAW_STATS
	currNumDrawnVerts += getNumVerts();
	currNumDrawCalls++;
#endif
	if (_mode != PrimitiveType::None) {
		mode = _mode;
	}

	draw_(g, mode, instances);
}
