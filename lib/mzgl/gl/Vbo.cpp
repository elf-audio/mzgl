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

void Vbo::Buffer::deallocate() {
	if (id != 0) {
		glDeleteBuffers(1, &id);
		id = 0;
	}
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
std::vector<Vbo *> Vbo::vbos;
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
	glGenVertexArrays(1, &vertexArrayObject);

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
	if (vertexArrayObject != 0) glDeleteVertexArrays(1, &vertexArrayObject);
	vertexArrayObject = 0;

	mode = PrimitiveType::None;
}

void Vbo::clear() {
	deallocateResources();
	glGenVertexArrays(1, &vertexArrayObject);
}

template <class T>
static void setBuffer(Vbo &vbo, Vbo::Buffer &buff, const std::vector<T> &v) {
	if (buff.id != 0) glDeleteBuffers(1, &buff.id);

	glBindVertexArray(vbo.vertexArrayObject);

	buff.dimensions = sizeof(T) / 4;
	buff.size		= v.size();
	glGenBuffers(1, &buff.id);
	glBindBuffer(GL_ARRAY_BUFFER, buff.id);
	glBufferData(GL_ARRAY_BUFFER, buff.size * sizeof(T), v.data(), GL_STATIC_DRAW);
}

template <class T>
static void updateVertBuffer(const uint32_t &vao, Vbo::Buffer &buff, const std::vector<T> &data) {
	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, buff.id);
	glBufferSubData(GL_ARRAY_BUFFER, 0, buff.size * sizeof(float) * buff.dimensions, data.data());
}

template <class T>
static Vbo &setVboVertices(Vbo &vbo, const std::vector<T> &data) {
	if (data.size() == 0) {
		Log::e() << "Trying to setVertices with no vertices";
		return vbo;
	}
	bool updating = false;
	if (data.size() == vbo.vertexBuffer.size && vbo.vertexBuffer.id != 0
		&& vbo.vertexBuffer.dimensions == sizeof(T) / 4)
		updating = true;

	if (updating) updateVertBuffer(vbo.vertexArrayObject, vbo.vertexBuffer, data);
	else setBuffer(vbo, vbo.vertexBuffer, data);
	return vbo;
}
Vbo &Vbo::setVertices(const std::vector<vec2> &verts) {
	return setVboVertices(*this, verts);
}
Vbo &Vbo::setVertices(const std::vector<vec3> &verts) {
	return setVboVertices(*this, verts);
}
Vbo &Vbo::setVertices(const std::vector<vec4> &verts) {
	return setVboVertices(*this, verts);
}

Vbo &Vbo::setColors(const std::vector<vec4> &cols) {
	setBuffer(*this, colorbuffer, cols);
	return *this;
}

Vbo &Vbo::setTexCoords(const std::vector<vec2> &tcs) {
	setBuffer(*this, texCoordBuffer, tcs);
	return *this;
}

Vbo &Vbo::setNormals(const std::vector<vec3> &norms) {
	setBuffer(*this, normalBuffer, norms);
	return *this;
}

Vbo &Vbo::setIndices(const std::vector<unsigned int> &indices) {
	if (indexBuffer.id != 0) glDeleteBuffers(1, &indexBuffer.id);

	glBindVertexArray(vertexArrayObject);
	indexBuffer.size = indices.size();

	glGenBuffers(1, &indexBuffer.id);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer.id);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
	return *this;
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

static void bindBuffer(Vbo::Buffer &buffer, int attribute) {
	glEnableVertexAttribArray(attribute);
	glBindBuffer(GL_ARRAY_BUFFER, buffer.id);
	glVertexAttribPointer(attribute,
						  buffer.dimensions, // size
						  GL_FLOAT, // type
						  GL_FALSE, // normalized?
						  0, // stride
						  (void *) 0 // array buffer offset
	);
	GetError();
}

static void checkAndBindBuffer(Vbo::Buffer &buffer, int attribute) {
	if (buffer.id == 0) return;

	if (attribute == -1) {
		Log::e()
			<< "There is a missing attribute in this shader maybe? - are you using Drawer and not setting ignoreColor = true?";
		return;
	}
	bindBuffer(buffer, attribute);
}

void Vbo::draw(Graphics &g, vec2 offset) {
	ScopedTranslate t(g, offset);
	draw(g);
}

void Vbo::chooseShaderAndSetDefaults(Graphics &g) {
	// TODO: I don't know if this would switch between different default shaders
	if (g.currShader == nullptr || g.currShader->isDefaultShader) {
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
}
void Vbo::draw(Graphics &g, PrimitiveType _mode, size_t instances) {
	if (vertexBuffer.size == 0) {
		return;
	}

	if (vertexArrayObject == 0) {
		return;
	}

#ifdef DO_DRAW_STATS
	_numDrawnVerts += vertexBuffer.size;
	_numDrawCalls++;
#endif
	if (_mode == PrimitiveType::None) {
		if (this->mode != PrimitiveType::None) {
			_mode = this->mode;
		} else {
			_mode = PrimitiveType::Triangles;
		}
	}

	chooseShaderAndSetDefaults(g);

	glBindVertexArray(vertexArrayObject);

	bindBuffer(vertexBuffer, g.currShader->positionAttribute);

	checkAndBindBuffer(colorbuffer, g.currShader->colorAttribute);
	checkAndBindBuffer(texCoordBuffer, g.currShader->texCoordAttribute);
	checkAndBindBuffer(normalBuffer, g.currShader->normAttribute);

	if (indexBuffer.id != 0) {
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer.id);
	}

	auto glMode = primitiveTypeToGLMode(_mode);
	if (instances > 1) {
		if (indexBuffer.id) {
			glDrawElementsInstanced(glMode, (GLsizei) indexBuffer.size, GL_UNSIGNED_INT, nullptr, instances);
		} else {
			glDrawArraysInstanced(glMode, 0, (GLsizei) vertexBuffer.size, instances);
		}
	} else {
		if (indexBuffer.id) {
			glDrawElements(glMode, (GLsizei) indexBuffer.size, GL_UNSIGNED_INT, nullptr);
		} else {
			glDrawArrays(glMode, 0, (GLsizei) vertexBuffer.size);
		}
	}

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	glBindVertexArray(0);

	glDisableVertexAttribArray(g.currShader->positionAttribute);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	if (colorbuffer.id != 0 && g.currShader->colorAttribute != -1) {
		glDisableVertexAttribArray(g.currShader->colorAttribute);
	}
	if (normalBuffer.id != 0 && g.currShader->normAttribute != -1) {
		glDisableVertexAttribArray(g.currShader->normAttribute);
	}

	if (texCoordBuffer.id != 0 && g.currShader->texCoordAttribute != -1) {
		glDisableVertexAttribArray(g.currShader->texCoordAttribute);
	}
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
