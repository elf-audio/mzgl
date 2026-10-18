#pragma once

// Vbo for the native D3D11 backend. Vertex data is retained CPU-side. At draw
// time it goes one of two ways:
//
//  - streamed: appended to the backend's per-device stream ring (one dynamic
//    buffer, MAP_WRITE_NO_OVERWRITE) and bound with a byte offset. Used for
//    pooled Vbos (Vbo::createFromPool, re-set every frame), for the first
//    draw after any set*() call, and for data too big for the ring never.
//    No buffer object is created, so per-frame geometry never churns GPU
//    buffers.
//  - static: data drawn again in a later frame without having changed is
//    promoted to an IMMUTABLE buffer, created once and reused until the data
//    changes (then streamed again). So static geometry costs one buffer and
//    one upload, exactly like sokol's immutable buffers, and changing
//    geometry costs no buffer at all.
//
// Index data follows the same scheme (the ring has the index bind flag too).
//
// Pure C++ header - the ID3D11Buffer handles are opaque void*s managed by
// helpers in D3D11Vbo.cpp.

#include <glm/glm.hpp>
#include <vector>
#include "Vbo.h"
#include "sokol_gfx.h" // sg_vertex_format only (shared with the shader reflection)

class Graphics;
class D3D11Shader;

// release an ID3D11Buffer held as void*; null-safe, defined in D3D11Vbo.cpp
void mzglD3D11BufferRelease(void *&buf);

class D3D11Vbo : public Vbo {
public:
	~D3D11Vbo() override { deallocate(); }

	// CPU copy + lazily created GPU buffer + the record of where the data was
	// last streamed to (valid while the ring epoch and frame match)
	struct GpuData {
		std::vector<uint8_t> cpuData;
		void *gpu			   = nullptr; // ID3D11Buffer*, IMMUTABLE, created at the promoting draw
		uint32_t gpuGeneration = 0; // device generation the buffer belongs to
		bool gpuDirty		   = false;
		uint32_t streamOffset  = 0;
		uint32_t streamEpoch   = 0;
		uint32_t streamFrame   = 0;
		bool streamed		   = false;

		[[nodiscard]] bool present() const { return !cpuData.empty(); }

		void markChanged() {
			mzglD3D11BufferRelease(gpu);
			gpuDirty = true;
			streamed = false;
		}
		void releaseGpu() {
			mzglD3D11BufferRelease(gpu);
			gpuDirty = false;
			streamed = false;
		}
	};

	struct Buffer : GpuData {
		int size	 = 0; // element count
		int unitSize = 0; // bytes per element
		bool isIndex = false;

		template <typename T>
		void set(const std::vector<T> &v) {
			isIndex = std::is_integral_v<T>;
			cpuData.assign((const uint8_t *) v.data(), (const uint8_t *) v.data() + v.size() * sizeof(T));
			size	 = (int) v.size();
			unitSize = (int) sizeof(T);
			markChanged();
		}

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
			releaseGpu();
			cpuData.clear();
			size = 0;
		}

		~Buffer() { mzglD3D11BufferRelease(gpu); }
	};

	Buffer positionBuffer;
	Buffer colorBuffer;
	Buffer texCoordBuffer;
	Buffer indexBuffer;
	Buffer instanceIndexBuffer;

	// Interleaved single-buffer path, built by setGeometry() - same layout
	// scheme as SokolVbo (pos[+col][+uv] packed per-vertex, indices separate).
	struct Interleaved : GpuData {
		int vertCount	= 0;
		int strideBytes = 0;
		int colOffset	= -1; // byte offset within vertex, -1 if absent
		int uvOffset	= -1;

		void deallocate() {
			releaseGpu();
			cpuData.clear();
			vertCount = 0;
		}
		~Interleaved() { mzglD3D11BufferRelease(gpu); }
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

	// Vbo::createFromPool: always stream, never promote to a static buffer
	// (the data is replaced every frame anyway)
	void setStreaming(bool s) { streaming = s; }
	[[nodiscard]] bool isStreaming() const { return streaming; }

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
	bool streaming = false;
	D3D11Shader *getShader(Graphics &g) const;
	void drawInterleaved(Graphics &g, PrimitiveType mode, size_t numInstances);
};
