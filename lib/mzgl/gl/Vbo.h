//
//  Vbo.h
//  MZGL
//
//  Created by Marek Bereza on 15/01/2018.
//  Copyright © 2018 Marek Bereza. All rights reserved.
//

#pragma once

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

// Global switch for deferred (lazy) GPU-buffer upload, OFF by default (data
// uploads immediately at set time). When enabled, vertex data is kept CPU-side
// and only uploaded when a Vbo is first drawn, and GPU buffers idle for a few
// seconds are evicted (recreated transparently on the next draw) - useful when
// many plugin instances share sokol's global buffer pool. Sokol builds only -
// a no-op on OpenGL. Toggle at startup; flipping mid-run is safe but only
// affects data set from then on.
void setVboLazyUploadEnabled(bool enabled);
bool isVboLazyUploadEnabled();

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

	// Creates a VBO backed by a buffer pool for efficient reuse.
	// Use this for temporary VBOs that are created and destroyed every frame.
	static VboRef createFromPool(Graphics &g);

	virtual ~Vbo();

	virtual Vbo &setVertices(const std::vector<vec2> &verts) = 0;
	virtual Vbo &setVertices(const std::vector<vec3> &verts) = 0;
	virtual Vbo &setVertices(const std::vector<vec4> &verts) = 0;

	virtual Vbo &setColors(const std::vector<vec4> &cols)											= 0;
	virtual Vbo &setTexCoords(const std::vector<vec2> &tcs)											= 0;
	virtual Vbo &setIndices(const std::vector<unsigned int> &indices)								= 0;
	virtual size_t getNumVerts()																	= 0;
	virtual void draw_(Graphics &g, PrimitiveType mode = PrimitiveType::None, size_t instances = 1) = 0;

	virtual void deallocate() = 0;
	////////////////////////////////////////////
	virtual Vbo &setGeometry(const Geometry &geom);
	Vbo &setMode(PrimitiveType mode);

	// if primitive type is none, it will use whatever PrimitiveType stored in the vbo
	void draw(Graphics &g, vec2 offset);
	void drawInstanced(Graphics &g, size_t instances) { draw(g, PrimitiveType::None, instances); };
	void draw(Graphics &g, PrimitiveType mode = PrimitiveType::None, size_t instances = 1);

#ifdef __ANDROID__
	static std::vector<Vbo *> vbos;
#endif

protected:
	Vbo();

private:
	PrimitiveType mode = PrimitiveType::Triangles;
};
