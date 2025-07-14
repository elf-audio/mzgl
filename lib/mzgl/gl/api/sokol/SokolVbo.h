#pragma once

#include "sokol_gfx.h"
#include <glm/glm.hpp>
#include <vector>
#include "SokolShader.h"
#include "Vbo.h"
class Graphics;
class Shader;
class SokolVbo : public Vbo {
public:
	void clear() {}

	class Buffer {
	public:
		sg_buffer buffer {.id = 0};

		int size	 = 0;
		int unitSize = 0;
		template <typename T>
		void set(const std::vector<T> &v) {
			deallocate();
			//        sg_update_buffer(vbuf_col, sg_range{colors.data(), colors.size() * sizeof(vec4)});
			sg_buffer_desc desc = {.data = sg_range {v.data(), v.size() * sizeof(T)}};

			if constexpr (std::is_integral_v<T>) {
				desc.type = SG_BUFFERTYPE_INDEXBUFFER;
			}

			buffer = sg_make_buffer(desc);

			size	 = v.size();
			unitSize = sizeof(T);
		}

		sg_vertex_format getFormat() const {
			// no int index yet.

			switch (unitSize) {
				case 4: return SG_VERTEXFORMAT_FLOAT;
				case 8: return SG_VERTEXFORMAT_FLOAT2;
				case 12: return SG_VERTEXFORMAT_FLOAT3;
				case 16: return SG_VERTEXFORMAT_FLOAT4;
				default: return SG_VERTEXFORMAT_INVALID;
			}
		}
		[[nodiscard]] bool valid() const { return buffer.id != 0; }
		void deallocate();

		~Buffer() { deallocate(); }
	};

	Buffer positionBuffer;
	Buffer colorBuffer;
	Buffer texCoordBuffer;
	Buffer normalBuffer;
	Buffer indexBuffer;

	Buffer instanceIndexBuffer;

	size_t getNumVerts() override { return positionBuffer.valid() && positionBuffer.size; }
	void draw_(Graphics &g, PrimitiveType mode = PrimitiveType::None, size_t instances = 1) override;

	void deallocate() override {
		positionBuffer.deallocate();
		colorBuffer.deallocate();
		texCoordBuffer.deallocate();
		normalBuffer.deallocate();
		indexBuffer.deallocate();
		instanceIndexBuffer.deallocate();
	}

	Vbo &setVertices(const std::vector<vec2> &verts) override {
		
		if (verts.size() == 0) return *this;
		positionBuffer.set(verts);
		return *this;
	}
	Vbo &setVertices(const std::vector<vec3> &verts) override {
		positionBuffer.set(verts);
		return *this;
	}
	Vbo &setVertices(const std::vector<vec4> &verts) override {
		positionBuffer.set(verts);
		return *this;
	}
	Vbo &setTexCoords(const std::vector<glm::vec2> &texCoords) override {
		texCoordBuffer.set(texCoords);
		return *this;
	}
	Vbo &setColors(const std::vector<glm::vec4> &colors) override {
		colorBuffer.set(colors);
		return *this;
	}

	Vbo &setNormals(const std::vector<glm::vec3> &normals) override {
		normalBuffer.set(normals);
		return *this;
	}

	Vbo &setIndices(const std::vector<uint32_t> &indices) override {
		// TODO: remove me
		if (indices.size() == 0) return *this;
		indexBuffer.set(indices);
		return *this;
	}

private:
	SokolShader *getShader(Graphics &g) const;
};
