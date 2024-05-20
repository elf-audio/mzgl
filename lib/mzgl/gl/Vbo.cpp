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
#include "Graphics.h"
#include "util.h"
#include "Geometry.h"

using namespace std;

void Vbo::Buffer::upload() {
}

void Vbo::Buffer::deallocate() {
	if (id != 0) glDeleteBuffers(1, &id);
	size = 0;
}

int Vbo::_numDrawnVerts = 0;
int Vbo::_numDrawCalls	= 0;
int Vbo::numDrawnVerts	= 0;
int Vbo::numDrawCalls	= 0;
void Vbo::resetDrawStats() {
	numDrawnVerts  = _numDrawnVerts;
	numDrawCalls   = _numDrawCalls;
	_numDrawnVerts = 0;
	_numDrawCalls  = 0;
}
// TODO BUG: this is all stupid, deleteing and generating vertex arrays etc.
#ifdef __ANDROID__
vector<Vbo *> Vbo::vbos;
void Vbo::printVbos() {
	Log::e() << "BEGIN -----------------------------------------";
	for (int i = 0; i < vbos.size(); i++) {
		//Log::e() << "Vbo ("<<i<<") id: " << vbos[i]->vertexArrayObject;
		Log::d() << vbos[i]->vertexArrayObject << ", " << vbos[i]->vertexBuffer.id << ", "
				 << vbos[i]->colorbuffer.id << ", " << vbos[i]->texCoordBuffer.id << ", "
				 << vbos[i]->normalBuffer.id << ", " << vbos[i]->indexBuffer.id;
	}
	Log::e() << "END -----------------------------------------";
}
#endif

Vbo::Vbo() {
#ifndef MZGL_GL2
	glGenVertexArrays(1, &vertexArrayObject);
#endif

#ifdef __ANDROID__
	vbos.push_back(this);
#endif
}
Vbo::~Vbo() {
	deallocateResources();

#ifdef __ANDROID__
	for (int i = 0; i < vbos.size(); i++) {
		if (vbos[i] == this) {
			vbos.erase(vbos.begin() + i);
			break;
		}
	}
#endif
}

void Vbo::deallocateResources() {
	vertexBuffer.deallocate();
	colorbuffer.deallocate();
	texCoordBuffer.deallocate();
	normalBuffer.deallocate();
	indexBuffer.deallocate();
#ifndef MZGL_GL2
	if (vertexArrayObject != 0) glDeleteVertexArrays(1, &vertexArrayObject);
#endif

#ifndef MZGL_GL2
	vertexArrayObject = 0;
#endif
	mode = PrimitiveType::None;
}

void Vbo::clear() {
	deallocateResources();
#ifndef MZGL_GL2
	glGenVertexArrays(1, &vertexArrayObject);
#endif
}

void Vbo::updateVertBuffer(const float *data) {
#ifndef MZGL_GL2
	glBindVertexArray(vertexArrayObject);
#endif
	glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer.id);
	glBufferSubData(GL_ARRAY_BUFFER, 0, vertexBuffer.size * sizeof(float) * vertexBuffer.dimensions, data);
}

void Vbo::generateVertBuffer(const float *data, size_t numVerts, int numDims) {
	if (vertexBuffer.id != 0) glDeleteBuffers(1, &vertexBuffer.id);

	vertexBuffer.dimensions = numDims;
	vertexBuffer.size		= numVerts;
#ifndef MZGL_GL2
	glBindVertexArray(vertexArrayObject);
#endif
	glGenBuffers(1, &vertexBuffer.id);
	glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer.id);
	glBufferData(
		GL_ARRAY_BUFFER, vertexBuffer.size * sizeof(float) * vertexBuffer.dimensions, data, GL_STATIC_DRAW);
	GetError();
}

Vbo &Vbo::setTexCoords(const vector<vec2> &tcs) {
	if (texCoordBuffer.id != 0) glDeleteBuffers(1, &texCoordBuffer.id);
#ifndef MZGL_GL2
	glBindVertexArray(vertexArrayObject);
#endif

	texCoordBuffer.size = tcs.size();
	glGenBuffers(1, &texCoordBuffer.id);
	glBindBuffer(GL_ARRAY_BUFFER, texCoordBuffer.id);
	glBufferData(GL_ARRAY_BUFFER, tcs.size() * sizeof(vec2), tcs.data(), GL_STATIC_DRAW);

	return *this;
}

Vbo &Vbo::setNormals(const vector<vec3> &norms) {
	if (normalBuffer.id != 0) glDeleteBuffers(1, &normalBuffer.id);

#ifndef MZGL_GL2
	glBindVertexArray(vertexArrayObject);
#endif

	normalBuffer.size = norms.size();
	glGenBuffers(1, &normalBuffer.id);
	glBindBuffer(GL_ARRAY_BUFFER, normalBuffer.id);
	glBufferData(GL_ARRAY_BUFFER, norms.size() * sizeof(vec3), norms.data(), GL_STATIC_DRAW);

	return *this;
}

Vbo &Vbo::setIndices(const vector<unsigned int> &indices) {
	if (indexBuffer.id != 0) glDeleteBuffers(1, &indexBuffer.id);

#ifndef MZGL_GL2
	glBindVertexArray(vertexArrayObject);
#endif
	indexBuffer.size = indices.size();

	glGenBuffers(1, &indexBuffer.id);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer.id);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
	return *this;
}

void Vbo::generateColorBuffer(const float *data, size_t numCols, int numDims) {
	if (colorbuffer.id != 0) glDeleteBuffers(1, &colorbuffer.id);

#ifndef MZGL_GL2
	glBindVertexArray(vertexArrayObject);
#endif

	glGenBuffers(1, &colorbuffer.id);
	glBindBuffer(GL_ARRAY_BUFFER, colorbuffer.id);
	colorbuffer.size	   = numCols;
	colorbuffer.dimensions = numDims;
	glBufferData(GL_ARRAY_BUFFER, numCols * sizeof(float) * colorbuffer.dimensions, data, GL_STATIC_DRAW);
}

static int primitiveTypeToGLMode(Vbo::PrimitiveType mode) {
	switch (mode) {
		case Vbo::PrimitiveType::Triangles: return GL_TRIANGLES;
		case Vbo::PrimitiveType::TriangleStrip: return GL_TRIANGLE_STRIP;
		case Vbo::PrimitiveType::TriangleFan: return GL_TRIANGLE_FAN;
		case Vbo::PrimitiveType::LineLoop: return GL_LINE_LOOP;
		case Vbo::PrimitiveType::LineStrip: return GL_LINE_STRIP;
		case Vbo::PrimitiveType::Lines: return GL_LINES;

		default: {
			Log::e() << "ERROR!! invalid primitive type " << (int) mode;
			return GL_TRIANGLES;
		}
	}
}

void Vbo::draw(Graphics &g, vec2 offset) {
	ScopedTranslate t(g, offset);
	draw(g);
}
void Vbo::draw(Graphics &g, PrimitiveType mode, size_t instances) {
	if (vertexBuffer.size == 0) {
		return;
	}

#ifndef MZGL_GL2
	if (vertexArrayObject == 0) {
		//        Log::e() << "Vertex is 0";
		return;
	}
#endif

#ifdef DO_DRAW_STATS
	_numDrawnVerts += vertexBuffer.size;
	_numDrawCalls++;
#endif
	if (mode == PrimitiveType::None) {
		if (this->mode != PrimitiveType::None) {
			mode = this->mode;
		} else {
			mode = PrimitiveType::Triangles;
		}
	}

	// TODO: I don't know if this would switch between different default shaders
	if (g.currShader == NULL || g.currShader->isDefaultShader) {
		if (colorbuffer.size == 0 && texCoordBuffer.size == 0) {
			g.nothingShader->begin();
		} else if (texCoordBuffer.size > 0 && colorbuffer.size == 0) {
			if (g.currShader == g.fontShader.get()) {
				g.fontShader->begin();
			} else {
				g.texShader->begin();
			}
		} else if (texCoordBuffer.size == 0 && colorbuffer.size > 0) {
			g.colorShader->begin();
		} else {
			g.colorTextureShader->begin();
		}
	}

	g.currShader->uniform("mvp", g.getMVP());
	if (g.currShader->needsColorUniform) {
		g.currShader->uniform("color", g.getColor());
	}

#ifndef MZGL_GL2
	glBindVertexArray(vertexArrayObject);
#endif

	GetError();
	glEnableVertexAttribArray(g.currShader->positionAttribute);
	GetError();

	glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer.id);
	GetError();

	glVertexAttribPointer(g.currShader->positionAttribute,
						  vertexBuffer.dimensions, // size
						  GL_FLOAT, // type
						  GL_FALSE, // normalized?
						  0, // stride
						  (void *) 0 // array buffer offset
	);
	GetError();

	if (colorbuffer.id != 0) {
		if (g.currShader->colorAttribute == -1) {
			Log::e()
				<< "There is no Color attribute in this shader - are you using Drawer and not setting ignoreColor = true?";
		} else {
			// 2nd attribute buffer : colors
			glEnableVertexAttribArray(g.currShader->colorAttribute);
			GetError();

			glBindBuffer(GL_ARRAY_BUFFER, colorbuffer.id);
			GetError();

			glVertexAttribPointer(g.currShader->colorAttribute,
								  colorbuffer.dimensions, // size
								  GL_FLOAT, // type
								  GL_FALSE, // normalized?
								  0, // stride
								  (void *) 0 // array buffer offset
			);
			GetError();
		}
	}

	if (texCoordBuffer.id != 0) {
		if (g.currShader->texCoordAttribute == -1) {
			Log::e() << "There is no TexCoord attribute in this shader";
		} else {
			glEnableVertexAttribArray(g.currShader->texCoordAttribute);
			GetError();

			glBindBuffer(GL_ARRAY_BUFFER, texCoordBuffer.id);
			GetError();

			glVertexAttribPointer(g.currShader->texCoordAttribute,
								  texCoordBuffer.dimensions, // size
								  GL_FLOAT, // type
								  GL_FALSE, // normalized?
								  0, // stride
								  (void *) 0 // array buffer offset
			);
			GetError();
		}
	}

	if (normalBuffer.id != 0) {
		if (g.currShader->normAttribute == -1) {
			Log::e() << "There is no Normal attribute in this shader";
		} else {
			glEnableVertexAttribArray(g.currShader->normAttribute);
			GetError();

			glBindBuffer(GL_ARRAY_BUFFER, normalBuffer.id);
			GetError();

			glVertexAttribPointer(g.currShader->normAttribute,
								  normalBuffer.dimensions, // size
								  GL_FLOAT, // type
								  GL_FALSE, // normalized?
								  0, // stride
								  (void *) 0 // array buffer offset
			);
			GetError();
		}
	}

	if (indexBuffer.id != 0) {
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer.id);
		GetError();
	}

	auto glMode = primitiveTypeToGLMode(mode);
	if (instances > 1) {
		if (indexBuffer.id) {
#ifdef MZGL_GL2
			// simulate instancing in gl2
			for (int i = 0; i < instances; i++) {
				g.currShader->setInstanceUniforms(i);
				glDrawElements(glMode, (GLsizei) indexBuffer.size, GL_UNSIGNED_INT, (void *) 0);
			}
#else
			glDrawElementsInstanced(glMode, (GLsizei) numIndices, GL_UNSIGNED_INT, (void *) 0, instances);
#endif
		} else {
#ifdef MZGL_GL2
			// simulate instancing in gl2
			for (int i = 0; i < instances; i++) {
				g.currShader->setInstanceUniforms(i);
				glDrawArrays(glMode, 0, (GLsizei) indexBuffer.size);
			}
#else
			glDrawArraysInstanced(glMode, 0, (GLsizei) numVerts, instances);
#endif
		}
	} else {
		if (indexBuffer.id) {
			glDrawElements(glMode, (GLsizei) indexBuffer.size, GL_UNSIGNED_INT, (void *) 0);
		} else {
			glDrawArrays(glMode, 0, (GLsizei) vertexBuffer.size);
		}
	}

	GetError();
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

#ifndef MZGL_GL2
	glBindVertexArray(0);
#endif

	glDisableVertexAttribArray(g.currShader->positionAttribute);
	GetError();
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	GetError();
	if (colorbuffer.id != 0 && g.currShader->colorAttribute != -1) {
		glDisableVertexAttribArray(g.currShader->colorAttribute);
	}
	GetError();
	if (normalBuffer.id != 0 && g.currShader->normAttribute != -1) {
		glDisableVertexAttribArray(g.currShader->normAttribute);
	}
	GetError();

	if (texCoordBuffer.id != 0 && g.currShader->texCoordAttribute != -1) {
		glDisableVertexAttribArray(g.currShader->texCoordAttribute);
	}
	GetError();
}

Vbo &Vbo::setMode(PrimitiveType mode) {
	this->mode = mode;
	return *this;
}

Vbo &Vbo::setGeometry(const Geometry &geom) {
	if (geom.cols.size() > 0) setColors(geom.cols);
	if (geom.verts.size() > 0) setVertices(geom.verts);
	if (geom.indices.size() > 0) setIndices(geom.indices);
	if (geom.texCoords.size() > 0) setTexCoords(geom.texCoords);
	return *this;
}
