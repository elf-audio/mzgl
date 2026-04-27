#pragma once
#include "sokol_gfx.h"
#include <vector>
#include <array>

// Pools sokol buffers by size bucket to avoid create/destroy churn.
// Buckets are powers of 2: 64, 128, 256, 512, 1K, 2K, 4K, 8K, 16K, 32K, 64K, 128K, 256K, 512K, 1M
//
// Why released buffers are not immediately re-acquirable:
// sokol's contract for SG_USAGE_STREAM buffers is that sg_update_buffer may be
// called at most once per frame on a given buffer. If the same pooled buffer is
// acquired -> updated -> drawn -> released -> acquired again within one frame
// (which the font and immediate-mode draw paths do many times per frame), the
// second sg_update_buffer overwrites the data the first draw still references,
// producing garbled rendering. D3D11's update path tolerates this; Metal does
// not. So released buffers go to a "pending" list and only become acquirable
// after the next sg_commit (registered via sg_add_commit_listener).
class SokolBufferPool {
public:
	struct PooledBuffer {
		sg_buffer buffer = {};
		int capacity	 = 0; // allocated size in bytes
	};

	// Get a buffer that can hold at least `size` bytes. May return a recycled buffer.
	// Returns {.capacity = 0} for oversized requests that can't be pooled.
	PooledBuffer acquire(int size, bool isIndex) {
		ensureCommitListener();
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

	// Return a buffer to the pool. It will become re-acquirable after the next sg_commit.
	void release(PooledBuffer buf, bool isIndex) {
		if (buf.buffer.id == 0) return;
		int bucket = bucketFor(buf.capacity);
		if (bucket < 0) {
			// Oversized buffer can't be pooled, just destroy it
			sg_destroy_buffer(buf.buffer);
			return;
		}
		auto &pending = isIndex ? indexPendingLists[bucket] : vertexPendingLists[bucket];
		pending.push_back(buf);
	}

	// Move buffers released during the just-finished frame into the free lists,
	// so they can be acquired and updated again. Driven by sg_commit (see
	// ensureCommitListener) rather than called manually, so every platform
	// (Metal, D3D11) gets the same behaviour with no render-loop changes.
	void endFrame() {
		for (int b = 0; b < NUM_BUCKETS; b++) {
			auto &fv = vertexFreeLists[b];
			auto &pv = vertexPendingLists[b];
			pooledCount += static_cast<int>(pv.size());
			fv.insert(fv.end(), pv.begin(), pv.end());
			pv.clear();

			auto &fi = indexFreeLists[b];
			auto &pi = indexPendingLists[b];
			pooledCount += static_cast<int>(pi.size());
			fi.insert(fi.end(), pi.begin(), pi.end());
			pi.clear();
		}
	}

	~SokolBufferPool() {
		// If sokol is already shut down, all GPU resources are already released.
		if (!sg_isvalid()) return;
		if (commitListenerAttached) {
			sg_remove_commit_listener(commitListener);
			commitListenerAttached = false;
		}
		auto destroyAll = [](auto &lists) {
			for (auto &list : lists)
				for (auto &b : list) sg_destroy_buffer(b.buffer);
		};
		destroyAll(vertexFreeLists);
		destroyAll(indexFreeLists);
		destroyAll(vertexPendingLists);
		destroyAll(indexPendingLists);
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

	void ensureCommitListener() {
		if (commitListenerAttached) return;
		if (!sg_isvalid()) return;
		commitListener.func		= [](void *userData) {
			static_cast<SokolBufferPool *>(userData)->endFrame();
		};
		commitListener.user_data = this;
		sg_add_commit_listener(commitListener);
		commitListenerAttached = true;
	}

	std::array<std::vector<PooledBuffer>, NUM_BUCKETS> vertexFreeLists;
	std::array<std::vector<PooledBuffer>, NUM_BUCKETS> indexFreeLists;
	std::array<std::vector<PooledBuffer>, NUM_BUCKETS> vertexPendingLists;
	std::array<std::vector<PooledBuffer>, NUM_BUCKETS> indexPendingLists;

	sg_commit_listener commitListener {};
	bool commitListenerAttached = false;
};
