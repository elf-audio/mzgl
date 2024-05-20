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
		TriangleFan,
		LineLoop,
		LineStrip,
		Lines,
		None,
	};

	// these were GLuints
	uint32_t vertexArrayObject = 0;
	uint32_t vertexBuffer	   = 0;
	uint32_t colorbuffer	   = 0;
	uint32_t texCoordBuffer	   = 0;
	uint32_t normalBuffer	   = 0;
	uint32_t indexBuffer	   = 0;

	static VboRef create() { return VboRef(new Vbo()); }

	virtual ~Vbo();

#ifdef __ANDROID__
	static std::vector<Vbo *> vbos;
	static void printVbos();
#endif
	void clear();

	template <typename T>
	Vbo &setVertices(const std::vector<T> &verts) {
		if (verts.size() == 0) {
			Log::e() << "Trying to setVertices with no vertices";
			return *this;
		}
		bool updating = false;
		if (verts.size() == numVerts && vertexBuffer != 0 && vertDimensions == sizeof(T) / 4) updating = true;

		if (updating) updateVertBuffer(&verts[0].x);
		else generateVertBuffer(&verts[0].x, verts.size(), 4);
		return *this;
	}

	template <typename T>
	Vbo &setColors(const std::vector<T> &cols) {
		if (cols.size() == 0) {
			Log::e() << "Trying to setColors with 0 colors";
			return *this;
		}
		generateColorBuffer(&cols[0].x, cols.size(), sizeof(T) / 4);
		return *this;
	}

	//	Vbo &setVertices(const std::vector<vec4> &verts);
	//	Vbo &setVertices(const std::vector<vec3> &verts);
	//	Vbo &setVertices(const std::vector<vec2> &verts);

	Vbo &setTexCoords(const std::vector<vec2> &tcs);

	//	Vbo &setColors(const std::vector<vec3> &cols);
	//	Vbo &setColors(const std::vector<vec4> &cols);

	Vbo &setNormals(const std::vector<vec3> &norms);
	Vbo &setIndices(const std::vector<unsigned int> &indices);

	Vbo &setGeometry(const Geometry &geom);
	//void addNormalizedTexCoords();

	// GL_TRIANGLES etc optional
	Vbo &setMode(PrimitiveType mode);

	// if primitive type is none, it will use whatever PrimitiveType stored in the vbo
	void draw(Graphics &g, PrimitiveType mode = PrimitiveType::None, size_t instances = 1);
	void draw(Graphics &g, vec2 offset);
	void drawInstanced(Graphics &g, size_t instances) { draw(g, PrimitiveType::None, instances); };
	size_t getNumVerts() { return numVerts; }

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

	size_t numVerts	  = 0;
	size_t numIndices = 0;
	size_t numCols	  = 0;

	size_t numTcs	= 0;
	size_t numNorms = 0;

	int vertDimensions = 0;
	int colDimensions  = 0;

	void generateVertBuffer(const float *data, size_t numVerts, int numDims);
	void updateVertBuffer(const float *data);

	void generateColorBuffer(const float *data, size_t numCols, int numDims);

	void deallocateResources();

private:
	Vbo();
};
