#pragma once

// Vbo for the native Metal backend. Vertex data is retained CPU-side; at draw
// time small attribute streams go through setVertexBytes (no buffer object at
// all) and larger ones lazily create an MTLBuffer which is then reused until
// the data changes. Index data always gets an MTLBuffer
// (drawIndexedPrimitives requires one).
//
// Pure C++ header - the MTLBuffer handles are opaque void*s managed by
// helpers in MetalVbo.mm.

#include <glm/glm.hpp>
#include <vector>
#include "Vbo.h"
#include "sokol_gfx.h" // sg_vertex_format only (shared with the shader reflection)

class Graphics;
class MetalShader;

// release a (__bridge_retained) id<MTLBuffer>; null-safe, defined in MetalVbo.mm
void mzglMetalBufferRelease(void *&buf);

class MetalVbo : public Vbo {
public:
	~MetalVbo() override { deallocate(); }

	struct Buffer {
		std::vector<uint8_t> cpuData;
		int size	 = 0; // element count
		int unitSize = 0; // bytes per element
		bool isIndex = false;
		void *gpu	 = nullptr; // retained id<MTLBuffer>, created lazily at draw
		bool gpuDirty = false;

		template <typename T>
		void set(const std::vector<T> &v) {
			isIndex = std::is_integral_v<T>;
			cpuData.assign((const uint8_t *) v.data(), (const uint8_t *) v.data() + v.size() * sizeof(T));
			size	 = (int) v.size();
			unitSize = (int) sizeof(T);
			gpuDirty = true;
		}

		[[nodiscard]] bool present() const { return !cpuData.empty(); }

		[[nodiscard]] sg_vertex_format getFormat() const {
			switch (unitSize) {
				case 4: return SG_VERTEXFORMAT_FLOAT;
				case 8: return SG_VERTEXFORMAT_FLOAT2;
				case 12: return SG_VERTEXFORMAT_FLOAT3;
				case 16: return SG_VERTEXFORMAT_FLOAT4;
				default: return SG_VERTEXFORMAT_INVALID;
			}
		}

		void deallocate() {
			mzglMetalBufferRelease(gpu);
			cpuData.clear();
			size	 = 0;
			gpuDirty = false;
		}

		~Buffer() { mzglMetalBufferRelease(gpu); }
	};

	Buffer positionBuffer;
	Buffer colorBuffer;
	Buffer texCoordBuffer;
	Buffer indexBuffer;
	Buffer instanceIndexBuffer;

	// Interleaved single-buffer path, built by setGeometry() - same layout
	// scheme as SokolVbo (pos[+col][+uv] packed per-vertex, indices separate).
	struct Interleaved {
		std::vector<uint8_t> cpuData;
		int vertCount	= 0;
		int strideBytes = 0;
		int colOffset	= -1; // byte offset within vertex, -1 if absent
		int uvOffset	= -1;
		void *gpu		= nullptr;
		bool gpuDirty	= false;

		[[nodiscard]] bool present() const { return !cpuData.empty(); }
		void deallocate() {
			mzglMetalBufferRelease(gpu);
			cpuData.clear();
			vertCount = 0;
			gpuDirty  = false;
		}
		~Interleaved() { mzglMetalBufferRelease(gpu); }
	};
	Interleaved il;
	bool interleaved = false;

	size_t getNumVerts() override {
		if (interleaved) return il.vertCount;
		return positionBuffer.present() ? positionBuffer.size : 0;
	}

	void draw_(Graphics &g, PrimitiveType mode = PrimitiveType::None, size_t instances = 1) override;

	Vbo &setGeometry(const Geometry &geom) override;

	void deallocate() override {
		positionBuffer.deallocate();
		colorBuffer.deallocate();
		texCoordBuffer.deallocate();
		indexBuffer.deallocate();
		instanceIndexBuffer.deallocate();
		il.deallocate();
		interleaved = false;
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
	Vbo &setIndices(const std::vector<uint32_t> &indices) override {
		if (indices.size() == 0) return *this;
		indexBuffer.set(indices);
		return *this;
	}

private:
	MetalShader *getShader(Graphics &g) const;
	void drawInterleaved(Graphics &g, PrimitiveType mode, size_t numInstances);
};
