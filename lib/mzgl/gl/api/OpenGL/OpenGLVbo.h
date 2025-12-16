#pragma once
#include "Vbo.h"
#include "mzOpenGL.h"

static int primitiveTypeToGLMode(Vbo::PrimitiveType mode) {
	switch (mode) {
		case Vbo::PrimitiveType::Triangles: return GL_TRIANGLES;
		case Vbo::PrimitiveType::TriangleStrip: return GL_TRIANGLE_STRIP;
		case Vbo::PrimitiveType::LineStrip: return GL_LINE_STRIP;
		case Vbo::PrimitiveType::Lines: return GL_LINES;

		default: {
			Log::e() << "ERROR!! invalid primitive type " << (int) mode;
			return GL_TRIANGLES;
		}
	}
}
#include "OpenGLShader.h"

class OpenGLVbo : public Vbo {
public:
	size_t getNumVerts() override { return vertexBuffer.size; }

	class Buffer {
	public:
		uint32_t id			= 0;
		uint32_t size		= 0;
		uint32_t dimensions = 0;
		[[nodiscard]] bool valid() const { return id != 0; }
		void deallocate() {
			if (id != 0) {
				glDeleteBuffers(1, &id);
				id = 0;
			}
			size = 0;
		}
	};

	class BufferBinding {
	public:
		BufferBinding(Buffer &buffer, int attribute)
			: buffer(buffer)
			, attribute(attribute) {
			if (!buffer.valid()) return;
			GetError();
			glEnableVertexAttribArray(attribute);
			GetError();
			glBindBuffer(GL_ARRAY_BUFFER, buffer.id);
			GetError();
			glVertexAttribPointer(attribute,
								  buffer.dimensions, // size
								  GL_FLOAT, // type
								  GL_FALSE, // normalized?
								  0, // stride
								  (void *) 0 // array buffer offset

			);
			GetError();
		}
		~BufferBinding() {
			if (!buffer.valid()) return;
			GetError();
			glDisableVertexAttribArray(attribute);
			GetError();
			glBindBuffer(GL_ARRAY_BUFFER, 0);
			GetError();
		}

	private:
		int attribute;
		Buffer &buffer;
	};

	uint32_t vertexArrayObject = 0;
	Buffer vertexBuffer;
	Buffer colorbuffer;
	Buffer texCoordBuffer;
	Buffer normalBuffer;
	Buffer indexBuffer;

	template <class T>
	void setBuffer(Buffer &buff, const std::vector<T> &v) {
		if (buff.id != 0) glDeleteBuffers(1, &buff.id);

		glBindVertexArray(vertexArrayObject);

		buff.dimensions = sizeof(T) / 4;
		buff.size		= v.size();
		glGenBuffers(1, &buff.id);
		glBindBuffer(GL_ARRAY_BUFFER, buff.id);
		glBufferData(GL_ARRAY_BUFFER, buff.size * sizeof(T), v.data(), GL_STATIC_DRAW);
	}

	template <class T>
	void updateVertBuffer(Buffer &buff, const std::vector<T> &data) {
		glBindVertexArray(vertexArrayObject);

		glBindBuffer(GL_ARRAY_BUFFER, buff.id);

		const GLsizeiptr bytes = data.size() * sizeof(T);
		glBufferSubData(GL_ARRAY_BUFFER, 0, bytes, data.data());
	}

	template <class T>
	void setVboVertices(const std::vector<T> &data) {
		if (data.size() == 0) {
			Log::e() << "Trying to setVertices with no vertices";
			return;
		}
		bool updating = false;
		if (data.size() == vertexBuffer.size && vertexBuffer.valid() && vertexBuffer.dimensions == sizeof(T) / 4)
			updating = true;

		if (updating) updateVertBuffer(vertexBuffer, data);
		else setBuffer(vertexBuffer, data);
	}

	OpenGLVbo()
		: Vbo() {
		glGenVertexArrays(1, &vertexArrayObject);
	}
	~OpenGLVbo() override { deallocate(); }

	void deallocate() override {
		vertexBuffer.deallocate();
		colorbuffer.deallocate();
		texCoordBuffer.deallocate();
		normalBuffer.deallocate();
		indexBuffer.deallocate();
		if (vertexArrayObject != 0) glDeleteVertexArrays(1, &vertexArrayObject);
		vertexArrayObject = 0;
	}

	Vbo &setVertices(const std::vector<vec2> &verts) override {
		setVboVertices(verts);
		return *this;
	}
	Vbo &setVertices(const std::vector<vec3> &verts) override {
		setVboVertices(verts);
		return *this;
	}
	Vbo &setVertices(const std::vector<vec4> &verts) override {
		setVboVertices(verts);
		return *this;
	}
	Vbo &setColors(const std::vector<vec4> &cols) override {
		setBuffer(colorbuffer, cols);
		return *this;
	}
	Vbo &setTexCoords(const std::vector<vec2> &tcs) override {
		setBuffer(texCoordBuffer, tcs);
		return *this;
	}
	Vbo &setNormals(const std::vector<vec3> &norms) override {
		setBuffer(normalBuffer, norms);
		return *this;
	}
	Vbo &setIndices(const std::vector<unsigned int> &indices) override {
		if (indexBuffer.id != 0) glDeleteBuffers(1, &indexBuffer.id);

		glBindVertexArray(vertexArrayObject);
		indexBuffer.size = indices.size();

		glGenBuffers(1, &indexBuffer.id);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer.id);
		glBufferData(
			GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
		return *this;
	}

	void bindAppropriateShader(Graphics &g, bool hasColor, bool hasTexCoords) {
		if (g.currShader != nullptr && !g.currShader->isDefaultShader) {
			return;
		}

		if (!hasTexCoords) {
			if (!hasColor) {
				g.nothingShader->begin();
			} else {
				g.colorShader->begin();
			}
		} else {
			if (!hasColor) {
				if (g.currShader == g.fontShader.get()) {
					g.fontShader->begin();
				} else {
					g.texShader->begin();
				}
			} else {
				g.colorTextureShader->begin();
			}
		}
	}
	void chooseShaderAndSetDefaults(Graphics &g) {
		bindAppropriateShader(g, colorbuffer.valid(), texCoordBuffer.valid());
		g.currShader->setMVP(g.getMVP());
		if (((OpenGLShader *) g.currShader)->needsColorUniform) {
			g.currShader->setColor(g.getColor());
		}
	}
	class VertexArrayBinding {
	public:
		VertexArrayBinding(uint32_t vao)
			: vao(vao) {
			if (vao != 0) {
				glBindVertexArray(vao);
			}
		}
		~VertexArrayBinding() {
			if (vao != 0) {
				glBindVertexArray(0);
			}
		}

	private:
		uint32_t vao;
	};
	void draw_(Graphics &g, Vbo::PrimitiveType mode, size_t instances) override {
		GetError();
		if (vertexBuffer.size == 0) {
			return;
		}

		if (vertexArrayObject == 0) {
			return;
		}

		GetError();
		chooseShaderAndSetDefaults(g);
		GetError();

		VertexArrayBinding vao(vertexArrayObject);

		auto shader = dynamic_cast<OpenGLShader *>(g.currShader);
		BufferBinding vertexBinding(vertexBuffer, shader->positionAttribute);
		BufferBinding colorBinding(colorbuffer, shader->colorAttribute);
		BufferBinding texCoordBinding(texCoordBuffer, shader->texCoordAttribute);
		BufferBinding normalBinding(normalBuffer, shader->normAttribute);
		if (indexBuffer.valid()) {
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer.id);
		}
		GetError();
		auto glMode = primitiveTypeToGLMode(mode);
		if (instances > 1) {
			if (indexBuffer.valid()) {
				glDrawElementsInstanced(glMode, (GLsizei) indexBuffer.size, GL_UNSIGNED_INT, nullptr, instances);
			} else {
				glDrawArraysInstanced(glMode, 0, (GLsizei) vertexBuffer.size, instances);
			}
		} else {
			if (indexBuffer.valid()) {
				glDrawElements(glMode, (GLsizei) indexBuffer.size, GL_UNSIGNED_INT, nullptr);
			} else {
				glDrawArrays(glMode, 0, (GLsizei) vertexBuffer.size);
			}
		}
		GetError();
		if (indexBuffer.valid()) {
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		}
		GetError();
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		GetError();
	}
};
