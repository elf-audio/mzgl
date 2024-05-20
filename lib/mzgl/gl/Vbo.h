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
	class Buffer {
	public:
		uint32_t id			= 0;
		uint32_t size		= 0;
		uint32_t dimensions = 0;
		void upload();
		void deallocate();
	};
	enum class PrimitiveType {
		Triangles,
		TriangleStrip,
		TriangleFan,
		LineLoop,
		LineStrip,
		Lines,
		None,
	};

	uint32_t vertexArrayObject;
	Buffer vertexBuffer;
	Buffer colorbuffer;
	Buffer texCoordBuffer;
	Buffer normalBuffer;
	Buffer indexBuffer;

	static VboRef create() { return VboRef(new Vbo()); }

	virtual ~Vbo();

#ifdef __ANDROID__
	static std::vector<Vbo *> vbos;
	static void printVbos();
#endif
	void clear();

	Vbo &setVertices(const std::vector<vec2> &verts);
	Vbo &setVertices(const std::vector<vec3> &verts);
	Vbo &setVertices(const std::vector<vec4> &verts);

	Vbo &setColors(const std::vector<vec4> &cols);
	Vbo &setTexCoords(const std::vector<vec2> &tcs);
	Vbo &setNormals(const std::vector<vec3> &norms);
	Vbo &setIndices(const std::vector<unsigned int> &indices);

	Vbo &setGeometry(const Geometry &geom);

	Vbo &setMode(PrimitiveType mode);

	// if primitive type is none, it will use whatever PrimitiveType stored in the vbo
	void draw(Graphics &g, PrimitiveType mode = PrimitiveType::None, size_t instances = 1);
	void draw(Graphics &g, vec2 offset);
	void drawInstanced(Graphics &g, size_t instances) { draw(g, PrimitiveType::None, instances); };
	size_t getNumVerts() { return vertexBuffer.size; }

#ifdef DO_DRAW_STATS
	static int numDrawnVerts;
	static int numDrawCalls;
	static void resetDrawStats();
#endif
private:
#ifdef DO_DRAW_STATS
	static int _numDrawnVerts;
	static int _numDrawCalls;
#endif

	PrimitiveType mode = PrimitiveType::None;

	void deallocateResources();

private:
	Vbo();
};
