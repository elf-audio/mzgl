#pragma once

#include "sokol_gfx.h"
#include <glm/glm.hpp>
#include <vector>
#include "SokolShader.h"
#include "SokolBufferPool.h"
#include "Vbo.h"

class Graphics;
class Shader;
class SokolVbo : public Vbo {
public:
	~SokolVbo() override = default;
	
	void clear() {}

	class Buffer {
	public:
		sg_buffer buffer = {};
		int size		 = 0;
		int unitSize	 = 0;

		SokolBufferPool *pool  = nullptr;
		int pooledCapacity	   = 0;
		bool isIndex		   = false;

		template <typename T>
		void set(const std::vector<T> &v, SokolBufferPool *bufPool = nullptr) {
			bool wasIndex = isIndex;
			isIndex		  = std::is_integral_v<T>;

			int dataSize = (int) (v.size() * sizeof(T));

			// If we have a pooled buffer that's big enough, reuse it
			if (pool && buffer.id != 0 && pooledCapacity >= dataSize && wasIndex == isIndex) {
				sg_update_buffer(buffer, {v.data(), (size_t) dataSize});
			} else {
				deallocate();
				pool = bufPool;
				if (pool) {
					auto pb		   = pool->acquire(dataSize, isIndex);
					buffer		   = pb.buffer;
					pooledCapacity = pb.capacity;
					sg_update_buffer(buffer, {v.data(), (size_t) dataSize});
				} else {
					// Fallback: create immutable buffer (no pool available)
					sg_buffer_desc desc = {};
					desc.data			= {v.data(), (size_t) dataSize};
					if (isIndex) desc.type = SG_BUFFERTYPE_INDEXBUFFER;
					buffer		   = sg_make_buffer(desc);
					pooledCapacity = 0;
				}
			}
			size	 = (int) v.size();
			unitSize = sizeof(T);
		}

		sg_vertex_format getFormat() const {
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

	void setPool(SokolBufferPool *p) { pool = p; }

	Vbo &setVertices(const std::vector<vec2> &verts) override {
		if (verts.size() == 0) return *this;
		positionBuffer.set(verts, pool);
		return *this;
	}
	Vbo &setVertices(const std::vector<vec3> &verts) override {
		positionBuffer.set(verts, pool);
		return *this;
	}
	Vbo &setVertices(const std::vector<vec4> &verts) override {
		positionBuffer.set(verts, pool);
		return *this;
	}
	Vbo &setTexCoords(const std::vector<glm::vec2> &texCoords) override {
		texCoordBuffer.set(texCoords, pool);
		return *this;
	}
	Vbo &setColors(const std::vector<glm::vec4> &colors) override {
		colorBuffer.set(colors, pool);
		return *this;
	}

	Vbo &setNormals(const std::vector<glm::vec3> &normals) override {
		normalBuffer.set(normals, pool);
		return *this;
	}

	Vbo &setIndices(const std::vector<uint32_t> &indices) override {
		if (indices.size() == 0) return *this;
		indexBuffer.set(indices, pool);
		return *this;
	}

	SokolBufferPool *pool = nullptr;

private:
	SokolShader *getShader(Graphics &g) const;
};
