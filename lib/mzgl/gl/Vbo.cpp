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
#include "maths.h"
#include "log.h"
#include "Graphics.h"
#include "util.h"
#include "Geometry.h"
#include "mzAssert.h"
#include "mainThread.h"

using namespace std;

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
		Log::d() << vbos[i]->vertexArrayObject << ", " << vbos[i]->vertexBuffer << ", " << vbos[i]->colorbuffer
				 << ", " << vbos[i]->texCoordBuffer << ", " << vbos[i]->normalBuffer << ", "
				 << vbos[i]->indexBuffer;
	}
	Log::e() << "END -----------------------------------------";
}
#endif

Vbo::Vbo() {
//	mzAssert(isMainThread());
#ifndef MZGL_GL2
	glGenVertexArrays(1, &vertexArrayObject);
#endif

#ifdef __ANDROID__
	vbos.push_back(this);
#endif
}
Vbo::~Vbo() {
	//Log::e() << "~Vbo()";
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
	if (vertexBuffer != 0) glDeleteBuffers(1, &vertexBuffer);
	if (colorbuffer != 0) glDeleteBuffers(1, &colorbuffer);
	if (texCoordBuffer != 0) glDeleteBuffers(1, &texCoordBuffer);
	if (normalBuffer != 0) glDeleteBuffers(1, &normalBuffer);
	if (indexBuffer != 0) glDeleteBuffers(1, &indexBuffer);
#ifndef MZGL_GL2
	if (vertexArrayObject != 0) glDeleteVertexArrays(1, &vertexArrayObject);
#endif

	vertexBuffer   = 0;
	colorbuffer	   = 0;
	texCoordBuffer = 0;
	normalBuffer   = 0;
	indexBuffer	   = 0;
#ifndef MZGL_GL2
	vertexArrayObject = 0;
#endif
	numVerts   = 0;
	numCols	   = 0;
	numIndices = 0;
	numTcs	   = 0;
	numNorms   = 0;

	mode = PrimitiveType::None;
}

void Vbo::clear() {
	deallocateResources();
#ifndef MZGL_GL2
	glGenVertexArrays(1, &vertexArrayObject);
#endif
}

Vbo &Vbo::setVertices(const vector<vec4> &verts) {
	if (verts.size() == 0) {
		Log::e() << "Trying to setVertices with no vertices";
		return *this;
	}
	bool updating = false;
	if (verts.size() == numVerts && vertexBuffer != 0 && vertDimensions == 4) updating = true;

	if (updating) updateVertBuffer(&verts[0].x);
	else generateVertBuffer(&verts[0].x, verts.size(), 4);
	return *this;
}

Vbo &Vbo::setVertices(const vector<vec3> &verts) {
	if (verts.size() == 0) {
		Log::e() << "Trying to setVertices with no vertices";
		return *this;
	}
	bool updating = false;
	if (verts.size() == numVerts && vertexBuffer != 0 && vertDimensions == 3) updating = true;

	if (updating) updateVertBuffer(&verts[0].x);
	else generateVertBuffer(&verts[0].x, verts.size(), 3);
	return *this;
}

Vbo &Vbo::setVertices(const vector<vec2> &verts) {
	if (verts.size() == 0) {
		Log::e() << "Trying to setVertices with no vertices";
		return *this;
	}
	bool updating = false;
	if (verts.size() == numVerts && vertexBuffer != 0 && vertDimensions == 2) updating = true;

	if (updating) updateVertBuffer(&verts[0].x);
	else generateVertBuffer(&verts[0].x, verts.size(), 2);
	return *this;
}

void Vbo::generateVertBuffer(const float *data, size_t numVerts, int numDims) {
	if (vertexBuffer != 0) {
		//printf("deleting vert buffer %d\n", vertexBuffer);
		glDeleteBuffers(1, &vertexBuffer);
		vertexBuffer = 0;
	}
	this->vertDimensions = numDims;
	this->numVerts		 = numVerts;
#ifndef MZGL_GL2
	glBindVertexArray(vertexArrayObject);
#endif
	glGenBuffers(1, &vertexBuffer);

	glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, numVerts * sizeof(float) * vertDimensions, data, GL_STATIC_DRAW);
	GetError();

	//printf("generating vert buffer: %d\n", vertexBuffer);
}

void Vbo::updateVertBuffer(const float *data) {
#ifndef MZGL_GL2
	glBindVertexArray(vertexArrayObject);
#endif
	glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
	glBufferSubData(GL_ARRAY_BUFFER, 0, numVerts * sizeof(float) * vertDimensions, data);
	//printf("updating vert buffer: %d\n", vertexBuffer);
}

Vbo &Vbo::setTexCoords(const vector<vec2> &tcs) {
	if (texCoordBuffer != 0) glDeleteBuffers(1, &texCoordBuffer);
#ifndef MZGL_GL2
	glBindVertexArray(vertexArrayObject);
#endif

	glGenBuffers(1, &texCoordBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, texCoordBuffer);
	glBufferData(GL_ARRAY_BUFFER, tcs.size() * sizeof(vec2), tcs.data(), GL_STATIC_DRAW);
	numTcs = tcs.size();
	return *this;
}

Vbo &Vbo::setNormals(const vector<vec3> &norms) {
	if (normalBuffer != 0) glDeleteBuffers(1, &normalBuffer);

#ifndef MZGL_GL2
	glBindVertexArray(vertexArrayObject);
#endif

	glGenBuffers(1, &normalBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, normalBuffer);
	glBufferData(GL_ARRAY_BUFFER, norms.size() * sizeof(vec3), norms.data(), GL_STATIC_DRAW);
	GetError();
	numNorms = norms.size();
	return *this;
}

Vbo &Vbo::setIndices(const vector<unsigned int> &indices) {
	numIndices = indices.size();
	if (indexBuffer != 0) glDeleteBuffers(1, &indexBuffer);

#ifndef MZGL_GL2
	glBindVertexArray(vertexArrayObject);
#endif

	glGenBuffers(1, &indexBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
	return *this;
}

Vbo &Vbo::setColors(const vector<vec3> &cols) {
	if (cols.size() == 0) {
		Log::e() << "Trying to setColors with 0 colors";
		return *this;
	}
	generateColorBuffer(&cols[0].x, cols.size(), 3);
	return *this;
}
Vbo &Vbo::setColors(const vector<vec4> &cols) {
	if (cols.size() == 0) {
		Log::e() << "Trying to setColors with 0 colors";
		return *this;
	}
	generateColorBuffer(&cols[0].x, cols.size(), 4);
	return *this;
}

void Vbo::generateColorBuffer(const float *data, size_t numCols, int numDims) {
	if (colorbuffer != 0) glDeleteBuffers(1, &colorbuffer);

#ifndef MZGL_GL2
	glBindVertexArray(vertexArrayObject);
#endif

	glGenBuffers(1, &colorbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, colorbuffer);
	this->numCols		= numCols;
	this->colDimensions = numDims;
	glBufferData(GL_ARRAY_BUFFER, numCols * sizeof(float) * colDimensions, data, GL_STATIC_DRAW);
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
void Vbo::draw(Graphics &g, PrimitiveType mode, size_t instances) {
	if (numVerts == 0) {
		//	    Log::d() << "This is so hard to find";
		//		Log::i() << "Trying to draw Vbo with no vertices";
		return;
	}

#ifndef MZGL_GL2
	if (vertexArrayObject == 0) {
		//        Log::e() << "Vertex is 0";
		return;
	}
#endif

#ifdef DO_DRAW_STATS
	_numDrawnVerts += numVerts;
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
		if (numCols == 0 && numTcs == 0) {
			g.nothingShader->begin();
		} else if (numTcs > 0 && numCols == 0) {
			if (g.currShader == g.fontShader.get()) {
				g.fontShader->begin();
			} else {
				g.texShader->begin();
			}
		} else if (numTcs == 0 && numCols > 0) {
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

	glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
	GetError();

	glVertexAttribPointer(g.currShader->positionAttribute,
						  vertDimensions, // size
						  GL_FLOAT, // type
						  GL_FALSE, // normalized?
						  0, // stride
						  (void *) 0 // array buffer offset
	);
	GetError();

	if (colorbuffer != 0) {
		if (g.currShader->colorAttribute == -1) {
			Log::e()
				<< "There is no Color attribute in this shader - are you using Drawer and not setting ignoreColor = true?";
		} else {
			// 2nd attribute buffer : colors
			glEnableVertexAttribArray(g.currShader->colorAttribute);
			GetError();

			glBindBuffer(GL_ARRAY_BUFFER, colorbuffer);
			GetError();

			glVertexAttribPointer(g.currShader->colorAttribute,
								  colDimensions, // size
								  GL_FLOAT, // type
								  GL_FALSE, // normalized?
								  0, // stride
								  (void *) 0 // array buffer offset
			);
			GetError();
		}
	}

	if (texCoordBuffer != 0) {
		if (g.currShader->texCoordAttribute == -1) {
			Log::e() << "There is no TexCoord attribute in this shader";
		} else {
			glEnableVertexAttribArray(g.currShader->texCoordAttribute);
			GetError();

			glBindBuffer(GL_ARRAY_BUFFER, texCoordBuffer);
			GetError();

			glVertexAttribPointer(g.currShader->texCoordAttribute,
								  2, // size
								  GL_FLOAT, // type
								  GL_FALSE, // normalized?
								  0, // stride
								  (void *) 0 // array buffer offset
			);
			GetError();
		}
	}

	if (normalBuffer != 0) {
		if (g.currShader->normAttribute == -1) {
			Log::e() << "There is no Normal attribute in this shader";
		} else {
			glEnableVertexAttribArray(g.currShader->normAttribute);
			GetError();

			glBindBuffer(GL_ARRAY_BUFFER, normalBuffer);
			GetError();

			glVertexAttribPointer(g.currShader->normAttribute,
								  3, // size
								  GL_FLOAT, // type
								  GL_FALSE, // normalized?
								  0, // stride
								  (void *) 0 // array buffer offset
			);
			GetError();
		}
	}

	if (indexBuffer != 0) {
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
		GetError();
	}

	auto glMode = primitiveTypeToGLMode(mode);
	if (instances > 1) {
		if (indexBuffer) {
#ifdef MZGL_GL2
			// simulate instancing in gl2
			for (int i = 0; i < instances; i++) {
				g.currShader->setInstanceUniforms(i);
				glDrawElements(glMode, (GLsizei) numIndices, GL_UNSIGNED_INT, (void *) 0);
			}
#else
			glDrawElementsInstanced(glMode, (GLsizei) numIndices, GL_UNSIGNED_INT, (void *) 0, instances);
#endif
		} else {
#ifdef MZGL_GL2
			// simulate instancing in gl2
			for (int i = 0; i < instances; i++) {
				g.currShader->setInstanceUniforms(i);
				glDrawArrays(glMode, 0, (GLsizei) numVerts);
			}
#else
			glDrawArraysInstanced(glMode, 0, (GLsizei) numVerts, instances);
#endif
		}
	} else {
		if (indexBuffer) {
			glDrawElements(glMode, (GLsizei) numIndices, GL_UNSIGNED_INT, (void *) 0);
		} else {
			glDrawArrays(glMode, 0, (GLsizei) numVerts);
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
	if (colorbuffer != 0 && g.currShader->colorAttribute != -1) {
		glDisableVertexAttribArray(g.currShader->colorAttribute);
	}
	GetError();
	if (normalBuffer != 0 && g.currShader->normAttribute != -1) {
		glDisableVertexAttribArray(g.currShader->normAttribute);
	}
	GetError();

	if (texCoordBuffer != 0 && g.currShader->texCoordAttribute != -1) {
		glDisableVertexAttribArray(g.currShader->texCoordAttribute);
	}
	GetError();
}

Vbo &Vbo::setMode(PrimitiveType mode) {
	this->mode = mode;
	return *this;
}

Vbo &Vbo::setGeometry(const Geometry &geom) {
	if (geom.cols.size() > 0) {
		setColors(geom.cols);
	}
	if (geom.verts.size() > 0) {
		setVertices(geom.verts);
	}
	if (geom.indices.size() > 0) {
		setIndices(geom.indices);
	}
	if (geom.texCoords.size() > 0) {
		setTexCoords(geom.texCoords);
	}
	return *this;
}
