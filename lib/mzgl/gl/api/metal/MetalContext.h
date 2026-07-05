#pragma once

// INTERNAL to the Metal backend - only include from the api/metal/*.mm files
// (and the Metal view). Holds the Objective-C side that the pure-C++ headers
// hide: the device/queue/per-frame encoder state and the process-global
// texture registry.

#ifndef __OBJC__
#	error "MetalContext.h is Objective-C++ only - include it from .mm files"
#endif

#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <cstdint>

// Vertex buffers bind at this index and above; the sokol-shdc MSL puts each
// stage's uniform block at [[buffer(0)]], so the vertex-attribute buffers
// must not collide. Shared by the pipeline vertex descriptor (MetalShader)
// and the encoder bindings (MetalVbo).
constexpr int mzglMetalVertexBufferIndexBase = 8;

// Metal's setVertexBytes/setFragmentBytes small-data limit - data at or
// below this size skips MTLBuffer creation entirely.
constexpr size_t mzglMetalSetBytesLimit = 4096;

// Recycles MTLBuffers across frames so per-frame geometry (>4KB, e.g. the
// grid keyboard's text or waveforms) doesn't create+destroy a GPU buffer
// every frame - buffer creation goes through the kernel (IOGPUResourceCreate)
// and dominated the idle render cost before pooling. Released buffers sit in
// `pending` until the frame's command buffer completes (an in-flight frame
// may still read them), then move to power-of-two free lists.
struct MetalBufferPool {
	// buffer with capacity >= size; contents undefined, caller memcpys.
	// Returned buffers are guaranteed GPU-idle.
	id<MTLBuffer> acquire(size_t size);

	// hand a buffer back; it becomes reusable after the current frame's
	// command buffer completes (see MetalAPI::endFrame)
	void release(id<MTLBuffer> buf);

	// grab the buffers released since the last frame boundary (endFrame
	// passes them to a command-buffer completion handler)
	std::vector<id<MTLBuffer>> takePending();

	// completion handler: make a batch reusable, respecting the size cap
	void recycle(std::vector<id<MTLBuffer>> batch);

	static MetalBufferPool &instance();

private:
	static size_t bucketFor(size_t n);
	std::mutex mut;
	std::unordered_map<size_t, std::vector<id<MTLBuffer>>> freeLists; // keyed by capacity
	std::vector<id<MTLBuffer>> pending;
	size_t freeBytes						= 0;
	static constexpr size_t kMaxFreeBytes	= 32 * 1024 * 1024;
	static constexpr size_t kMinBucketBytes = 4096;
};

struct MetalAPIImpl {
	// device/queue are adopted from the process-global device on first use
	id<MTLDevice> device	  = nil;
	id<MTLCommandQueue> queue = nil;

	// per-frame state, valid between beginFrame and endFrame
	id<MTLCommandBuffer> cmdBuffer		= nil;
	id<MTLRenderCommandEncoder> encoder = nil;
	id<CAMetalDrawable> drawable		= nil;
	int fbWidth							= 0;
	int fbHeight						= 0;

	// encoder state memos (reset each beginFrame): skip redundant
	// setRenderPipelineState / setFragmentTexture calls - the driver's
	// state-change cost per draw is measurable and UI draws come in long
	// runs of the same pipeline/texture
	void *curPipeline	   = nullptr; // unretained
	uint32_t curTexId	   = 0;
	uint32_t curSamplerKey = 0xffffffff;

	MTLPixelFormat colorFormat = MTLPixelFormatBGRA8Unorm;

	// sampler cache keyed by MetalTexture::samplerKey()
	std::unordered_map<uint32_t, id<MTLSamplerState>> samplers;
	id<MTLSamplerState> sampler(uint32_t key);
};

namespace mzglMetal {
	// the process-global device, created on first use; every MetalAPI
	// instance, texture and view must share it (resources are per-device)
	id<MTLDevice> device();

	// process-global texture registry: uint32_t handle -> MTLTexture
	// (mirrors sokol's image handles so texture ids keep working)
	uint32_t registerTexture(id<MTLTexture> tex);
	void replaceTexture(uint32_t idNum, id<MTLTexture> tex);
	void unregisterTexture(uint32_t idNum);
	id<MTLTexture> lookupTexture(uint32_t idNum);
} // namespace mzglMetal
