#pragma once

#include "sokol_gfx.h"
#include <glm/glm.hpp>
#include <vector>
#include "SokolShader.h"
#include "SokolBufferPool.h"
#include "SokolBufferTracker.h"
#include "Vbo.h"

class Graphics;
class Shader;

// Base for lazily-backed sokol buffers. The GPU buffer is only created when
// the buffer is first bound for a draw (ensure()), from a retained CPU copy,
// and is destroyed again after going unused for a few seconds (a sweep runs
// from a sokol commit listener). This keeps sokol's process-global buffer-pool
// slot usage proportional to what's actually being drawn, not to every Vbo the
// UI has ever created - crucial when many plugin instances share one process.
class SokolLazyBuffer {
public:
	virtual ~SokolLazyBuffer();
	// Destroy the GPU buffer if it hasn't been drawn since `cutoffMs`
	// (steady-clock ms). CPU data is kept so ensure() can recreate it.
	virtual void evictIfUnusedSince(int64_t cutoffMs) = 0;

	// Global switch for the lazy scheme, OFF by default (buffers upload to the
	// GPU immediately at set time). When on, unpooled buffers defer GPU-buffer
	// creation to first draw and an idle-eviction sweep frees ones unused for a
	// few seconds - useful when many plugin instances share sokol's global
	// buffer pool. Buffers deferred while it was on keep working - they still
	// create themselves on first draw. Toggle at startup; flipping mid-run is
	// safe but only affects buffers set from then on.
	static void setLazyModeEnabled(bool enabled) { lazyModeOn = enabled; }
	static bool lazyModeEnabled() { return lazyModeOn; }

protected:
	void registerLazy();   // idempotent, adds to the global sweep registry
	void unregisterLazy(); // called from destructor
	static int64_t nowMs();

private:
	static inline bool lazyModeOn = false;
	bool lazyRegistered			  = false;
};

class SokolVbo : public Vbo {
public:
	~SokolVbo() override = default;

	void clear() {}

	class Buffer : public SokolLazyBuffer {
	public:
		sg_buffer buffer = {};
		int size		 = 0;
		int unitSize	 = 0;

		SokolBufferPool *pool = nullptr;
		int pooledCapacity	  = 0;
		bool isIndex		  = false;

		// CPU copy retained for lazily (re)creating the GPU buffer at draw time.
		// Only used on the unpooled path; pooled (stream) buffers never populate it.
		std::vector<uint8_t> cpuData;
		int64_t lastUsedMs = 0;

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
					auto pb = pool->acquire(dataSize, isIndex);
					if (pb.capacity > 0) {
						buffer		   = pb.buffer;
						pooledCapacity = pb.capacity;
						sg_update_buffer(buffer, {v.data(), (size_t) dataSize});
					} else {
						// Too large to pool - fall through to the lazy unpooled path
						pool = nullptr;
						setLazy(v.data(), dataSize);
					}
				} else {
					setLazy(v.data(), dataSize);
				}
			}
			size	 = (int) v.size();
			unitSize = sizeof(T);
		}

		// Retain the data CPU-side; GPU buffer is created on first draw (ensure()).
		// With lazy mode off, uploads immediately instead (pre-lazy behaviour).
		void setLazy(const void *data, int dataSize) {
			pooledCapacity = 0;
			if (!lazyModeEnabled()) {
				cpuData.clear();
				sg_buffer_desc desc = {};
				desc.data			= {data, (size_t) dataSize};
				if (isIndex) desc.type = SG_BUFFERTYPE_INDEXBUFFER;
				buffer = sg_make_buffer(desc);
				SokolBufferTracker::track(buffer, dataSize, false);
				return;
			}
			cpuData.assign((const uint8_t *) data, (const uint8_t *) data + dataSize);
			registerLazy();
		}

		// Create the GPU buffer from cpuData if it doesn't exist. Call before binding.
		void ensure() {
			if (cpuData.empty()) return;
			lastUsedMs = nowMs();
			if (buffer.id != 0) return;
			sg_buffer_desc desc = {};
			desc.data			= {cpuData.data(), cpuData.size()};
			if (isIndex) desc.type = SG_BUFFERTYPE_INDEXBUFFER;
			buffer = sg_make_buffer(desc);
			SokolBufferTracker::track(buffer, (int) cpuData.size(), false);
		}

		void evictIfUnusedSince(int64_t cutoffMs) override {
			if (buffer.id == 0 || pool != nullptr || cpuData.empty()) return;
			if (lastUsedMs >= cutoffMs) return;
			if (sg_isvalid()) sg_destroy_buffer(buffer);
			SokolBufferTracker::untrack(buffer);
			buffer.id = 0;
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
		// GPU buffer currently exists (may be false for an evicted lazy buffer)
		[[nodiscard]] bool valid() const { return buffer.id != 0; }
		// Buffer conceptually has data (GPU-resident or retained CPU-side)
		[[nodiscard]] bool present() const { return buffer.id != 0 || !cpuData.empty(); }
		void deallocate();

		~Buffer() override {
			// unregister before teardown so a concurrent sweep can't touch a
			// half-destroyed object
			unregisterLazy();
			deallocate();
		}
	};

	Buffer positionBuffer;
	Buffer colorBuffer;
	Buffer texCoordBuffer;
	Buffer indexBuffer;

	Buffer instanceIndexBuffer;

	// Interleaved single-buffer path, built by setGeometry(). Holds pos(+col+uv)
	// packed per-vertex in one buffer; the index buffer stays separate (indexBuffer).
	// When `interleaved` is true the per-attribute Buffers above are unused.
	// Same lazy scheme as Buffer: data lives in cpuData until first drawn.
	struct Interleaved : public SokolLazyBuffer {
		sg_buffer buffer = {};
		int vertCount	 = 0;
		int strideBytes	 = 0;
		int colOffset	 = -1; // byte offset within vertex, -1 if absent
		int uvOffset	 = -1;
		std::vector<uint8_t> cpuData;
		int64_t lastUsedMs	  = 0;
		SokolBufferPool *pool = nullptr;
		int pooledCapacity	  = 0;

		bool valid() const { return buffer.id != 0; }
		bool present() const { return buffer.id != 0 || !cpuData.empty(); }

		void setLazy(const void *data, size_t dataSize) {
			deallocate();
			if (!lazyModeEnabled()) {
				sg_buffer_desc desc = {};
				desc.data			= {data, dataSize};
				buffer				= sg_make_buffer(desc);
				SokolBufferTracker::track(buffer, (int) dataSize, false);
				return;
			}
			cpuData.assign((const uint8_t *) data, (const uint8_t *) data + dataSize);
			registerLazy();
		}

		// Stream-buffer path for pooled VBOs that are re-set every frame
		// (Vbo::createFromPool). Reuses the same pooled buffer in place when it
		// fits - callers must not set+draw the same Vbo more than once per frame
		// (sokol allows one sg_update_buffer per buffer per frame).
		void setPooled(const void *data, size_t dataSize, SokolBufferPool *p) {
			if (pool && buffer.id != 0 && pooledCapacity >= (int) dataSize) {
				sg_update_buffer(buffer, {data, dataSize});
				return;
			}
			deallocate();
			pool	= p;
			auto pb = p->acquire((int) dataSize, false);
			if (pb.capacity > 0) {
				buffer		   = pb.buffer;
				pooledCapacity = pb.capacity;
				sg_update_buffer(buffer, {data, dataSize});
			} else {
				// too large to pool
				pool = nullptr;
				setLazy(data, dataSize);
			}
		}

		void ensure() {
			if (cpuData.empty()) return;
			lastUsedMs = nowMs();
			if (buffer.id != 0) return;
			sg_buffer_desc desc = {};
			desc.data			= {cpuData.data(), cpuData.size()};
			buffer				= sg_make_buffer(desc);
			SokolBufferTracker::track(buffer, (int) cpuData.size(), false);
		}

		void evictIfUnusedSince(int64_t cutoffMs) override {
			if (buffer.id == 0 || pool != nullptr || cpuData.empty()) return;
			if (lastUsedMs >= cutoffMs) return;
			if (sg_isvalid()) sg_destroy_buffer(buffer);
			SokolBufferTracker::untrack(buffer);
			buffer.id = 0;
		}

		void deallocate() {
			if (buffer.id != 0) {
				if (pool) {
					pool->release({buffer, pooledCapacity}, false);
				} else {
					if (sg_isvalid()) sg_destroy_buffer(buffer);
					SokolBufferTracker::untrack(buffer);
				}
				buffer.id	   = 0;
				pooledCapacity = 0;
			}
			cpuData.clear();
		}
		~Interleaved() override {
			unregisterLazy();
			deallocate();
		}
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

	Vbo &setIndices(const std::vector<uint32_t> &indices) override {
		if (indices.size() == 0) return *this;
		indexBuffer.set(indices, pool);
		return *this;
	}

	SokolBufferPool *pool = nullptr;

private:
	SokolShader *getShader(Graphics &g) const;
	void drawInterleaved(Graphics &g, PrimitiveType mode, size_t numInstances);
};
