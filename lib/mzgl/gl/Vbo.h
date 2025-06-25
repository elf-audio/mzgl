//
//  Vbo.h
//  MZGL
//
//  Created by Marek Bereza on 15/01/2018.
//  Copyright © 2018 Marek Bereza. All rights reserved.
//

#pragma once

#define DO_DRAW_STATS

#include <glm/glm.hpp>
#include "log.h"
// TODO: this should go somewhere more global
using vec2 = glm::vec2;
using vec3 = glm::vec3;
using vec4 = glm::vec4;

#include <vector>
#include <memory>

class Vbo;
using VboRef = std::shared_ptr<Vbo>;
class Graphics;
class Geometry;

class Vbo {
public:
	enum class PrimitiveType {
		Triangles,
		TriangleStrip,
		LineStrip,
		Lines,
		None,
	};

	static VboRef create();

	virtual ~Vbo();

	virtual Vbo &setVertices(const std::vector<vec2> &verts) = 0;
	virtual Vbo &setVertices(const std::vector<vec3> &verts) = 0;
	virtual Vbo &setVertices(const std::vector<vec4> &verts) = 0;

	virtual Vbo &setColors(const std::vector<vec4> &cols)											= 0;
	virtual Vbo &setTexCoords(const std::vector<vec2> &tcs)											= 0;
	virtual Vbo &setNormals(const std::vector<vec3> &norms)											= 0;
	virtual Vbo &setIndices(const std::vector<unsigned int> &indices)								= 0;
	virtual size_t getNumVerts()																	= 0;
	virtual void draw_(Graphics &g, PrimitiveType mode = PrimitiveType::None, size_t instances = 1) = 0;

	virtual void deallocate() = 0;
	////////////////////////////////////////////
	Vbo &setGeometry(const Geometry &geom);
	Vbo &setMode(PrimitiveType mode);

	// if primitive type is none, it will use whatever PrimitiveType stored in the vbo
	void draw(Graphics &g, vec2 offset);
	void drawInstanced(Graphics &g, size_t instances) { draw(g, PrimitiveType::None, instances); };
	void draw(Graphics &g, PrimitiveType mode = PrimitiveType::None, size_t instances = 1);
#ifdef DO_DRAW_STATS
	static int numDrawnVerts;
	static int numDrawCalls;
	static void resetDrawStats();
#endif

#ifdef __ANDROID__
	static std::vector<Vbo *> vbos;
#endif

protected:
	Vbo();

private:
	PrimitiveType mode = PrimitiveType::Triangles;
};
