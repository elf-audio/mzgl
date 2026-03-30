#pragma once
#include "sokol_gfx.h"
#include <vector>
#include <array>

// Pools D3D11 buffers by size bucket to avoid create/destroy churn.
// Buckets are powers of 2: 64, 128, 256, 512, 1K, 2K, 4K, 8K, 16K, 32K, 64K, 128K, 256K, 512K, 1M
class SokolBufferPool {
public:
	struct PooledBuffer {
		sg_buffer buffer = {};
		int capacity	 = 0; // allocated size in bytes
	};

	// Get a buffer that can hold at least `size` bytes. May return a recycled buffer.
	// Returns {.capacity = 0} for oversized requests that can't be pooled.
	PooledBuffer acquire(int size, bool isIndex) {
		int bucket = bucketFor(size);
		if (bucket < 0) return {}; // too large to pool
		auto &freeList = isIndex ? indexFreeLists[bucket] : vertexFreeLists[bucket];
		if (!freeList.empty()) {
			auto buf = freeList.back();
			freeList.pop_back();
			pooledCount--;
			return buf;
		}
		// Create a new buffer at bucket size
		int capacity		= bucketSize(bucket);
		sg_buffer_desc desc = {};
		desc.size			= capacity;
		desc.usage			= SG_USAGE_STREAM;
		if (isIndex) desc.type = SG_BUFFERTYPE_INDEXBUFFER;
		PooledBuffer pb;
		pb.buffer	= sg_make_buffer(desc);
		pb.capacity = capacity;
		totalCreated++;
		return pb;
	}

	// Return a buffer to the pool for reuse.
	void release(PooledBuffer buf, bool isIndex) {
		if (buf.buffer.id == 0) return;
		int bucket = bucketFor(buf.capacity);
		if (bucket < 0) {
			// Oversized buffer can't be pooled, just destroy it
			sg_destroy_buffer(buf.buffer);
			return;
		}
		auto &freeList = isIndex ? indexFreeLists[bucket] : vertexFreeLists[bucket];
		freeList.push_back(buf);
		pooledCount++;
	}

	~SokolBufferPool() {
		// If sokol is already shut down, all GPU resources are already released.
		if (!sg_isvalid()) return;
		for (auto &list : vertexFreeLists)
			for (auto &b : list) sg_destroy_buffer(b.buffer);
		for (auto &list : indexFreeLists)
			for (auto &b : list) sg_destroy_buffer(b.buffer);
	}

	int pooledCount	 = 0;
	int totalCreated = 0;

private:
	static constexpr int NUM_BUCKETS = 15; // 64B to 1MB

	static int bucketSize(int bucket) { return 64 << bucket; }

	static int bucketFor(int size) {
		// Find smallest bucket that fits. Returns -1 if too large to pool.
		for (int i = 0; i < NUM_BUCKETS; i++) {
			if (bucketSize(i) >= size) return i;
		}
		return -1;
	}

	std::array<std::vector<PooledBuffer>, NUM_BUCKETS> vertexFreeLists;
	std::array<std::vector<PooledBuffer>, NUM_BUCKETS> indexFreeLists;
};
