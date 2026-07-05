#include "MetalAPI.h"
#include "MetalContext.h"
#include "Graphics.h"
#include "SokolDefaultShaders.h"
#include <mutex>
#include <memory>
#include <utility>

// ---------------------------------------------------------------------------
// process-global device + texture registry
// ---------------------------------------------------------------------------
namespace mzglMetal {
	id<MTLDevice> device() {
		static id<MTLDevice> dev = MTLCreateSystemDefaultDevice();
		return dev;
	}

	namespace {
		struct TextureRegistry {
			std::mutex mut;
			std::unordered_map<uint32_t, id<MTLTexture>> textures;
			uint32_t nextId = 1;

			static TextureRegistry &instance() {
				// deliberately leaked - texture destructors can run at exit
				// time and must not touch a destroyed mutex
				static TextureRegistry *r = new TextureRegistry();
				return *r;
			}
		};
	} // namespace

	uint32_t registerTexture(id<MTLTexture> tex) {
		auto &r = TextureRegistry::instance();
		std::lock_guard<std::mutex> l(r.mut);
		uint32_t idNum	   = r.nextId++;
		r.textures[idNum] = tex;
		return idNum;
	}

	void replaceTexture(uint32_t idNum, id<MTLTexture> tex) {
		auto &r = TextureRegistry::instance();
		std::lock_guard<std::mutex> l(r.mut);
		r.textures[idNum] = tex;
	}

	void unregisterTexture(uint32_t idNum) {
		auto &r = TextureRegistry::instance();
		std::lock_guard<std::mutex> l(r.mut);
		r.textures.erase(idNum);
	}

	id<MTLTexture> lookupTexture(uint32_t idNum) {
		auto &r = TextureRegistry::instance();
		std::lock_guard<std::mutex> l(r.mut);
		auto it = r.textures.find(idNum);
		return it == r.textures.end() ? nil : it->second;
	}
} // namespace mzglMetal

// ---------------------------------------------------------------------------
// MetalBufferPool
// ---------------------------------------------------------------------------
MetalBufferPool &MetalBufferPool::instance() {
	// deliberately leaked - Vbo destructors can run at exit time and must not
	// touch a destroyed mutex
	static MetalBufferPool *p = new MetalBufferPool();
	return *p;
}

size_t MetalBufferPool::bucketFor(size_t n) {
	size_t b = kMinBucketBytes;
	while (b < n)
		b <<= 1;
	return b;
}

id<MTLBuffer> MetalBufferPool::acquire(size_t size) {
	const size_t bucket = bucketFor(size);
	{
		std::lock_guard<std::mutex> l(mut);
		auto it = freeLists.find(bucket);
		if (it != freeLists.end() && !it->second.empty()) {
			id<MTLBuffer> buf = it->second.back();
			it->second.pop_back();
			freeBytes -= bucket;
			return buf;
		}
	}
	return [mzglMetal::device() newBufferWithLength:bucket options:MTLResourceStorageModeShared];
}

void MetalBufferPool::release(id<MTLBuffer> buf) {
	if (buf == nil) return;
	std::lock_guard<std::mutex> l(mut);
	pending.push_back(buf);
}

std::vector<id<MTLBuffer>> MetalBufferPool::takePending() {
	std::lock_guard<std::mutex> l(mut);
	return std::exchange(pending, {});
}

void MetalBufferPool::recycle(std::vector<id<MTLBuffer>> batch) {
	std::lock_guard<std::mutex> l(mut);
	for (id<MTLBuffer> buf: batch) {
		const size_t cap = (size_t) buf.length;
		if (freeBytes + cap > kMaxFreeBytes) continue; // over budget - let ARC free it
		freeLists[cap].push_back(buf);
		freeBytes += cap;
	}
}

id<MTLSamplerState> MetalAPIImpl::sampler(uint32_t key) {
	auto it = samplers.find(key);
	if (it != samplers.end()) return it->second;

	MTLSamplerDescriptor *desc = [[MTLSamplerDescriptor alloc] init];
	const bool linear		   = (key & 1u) != 0;
	desc.minFilter			   = linear ? MTLSamplerMinMagFilterLinear : MTLSamplerMinMagFilterNearest;
	desc.magFilter			   = desc.minFilter;
	desc.sAddressMode = (key & 2u) ? MTLSamplerAddressModeRepeat : MTLSamplerAddressModeClampToEdge;
	desc.tAddressMode = (key & 4u) ? MTLSamplerAddressModeRepeat : MTLSamplerAddressModeClampToEdge;

	id<MTLSamplerState> s = [device newSamplerStateWithDescriptor:desc];
	samplers[key]		  = s;
	return s;
}

// ---------------------------------------------------------------------------
// MetalAPI
// ---------------------------------------------------------------------------
MetalAPI::MetalAPI(Graphics &g)
	: GraphicsAPI(g)
	, pimpl(std::make_unique<MetalAPIImpl>()) {
	pimpl->device = mzglMetal::device();
	pimpl->queue  = [pimpl->device newCommandQueue];
}

MetalAPI::~MetalAPI() = default;

void MetalAPI::init() {
	loadDefaultShaders();
}

void MetalAPI::loadDefaultShaders() {
	registerShaders(shaderRegistry);
	g.nothingShader		 = createDefaultShader("nothing", &g);
	g.colorShader		 = createDefaultShader("color", &g);
	g.colorTextureShader = createDefaultShader("colorTexture", &g);
	g.texShader			 = createDefaultShader("tex", &g);
	g.fontShader		 = createDefaultShader("font", &g);
	g.colorFontShader	 = createDefaultShader("colorFont", &g);
}

// the shader registry of the active backend, for generated shader-registration
// code (customShaders.h) that must work on both Sokol and Metal
SokolShaderRegistry &mzglShaderRegistry(Graphics &g) {
	return static_cast<MetalAPI &>(g.getAPI()).getShaderRegistry();
}

void MetalAPI::beginFrame(void *mtkViewPtr) {
	MTKView *view = (__bridge MTKView *) mtkViewPtr;

	MTLRenderPassDescriptor *rpd = view.currentRenderPassDescriptor;
	if (rpd == nil) return;

	pimpl->drawable	 = view.currentDrawable;
	pimpl->fbWidth	 = (int) view.drawableSize.width;
	pimpl->fbHeight	 = (int) view.drawableSize.height;
	pimpl->cmdBuffer = [pimpl->queue commandBuffer];
	pimpl->encoder	 = [pimpl->cmdBuffer renderCommandEncoderWithDescriptor:rpd];

	pimpl->curPipeline	  = nullptr;
	pimpl->curTexId		  = 0;
	pimpl->curSamplerKey  = 0xffffffff;

	maskIsOn = false;
	maskRect = Rectf(0, 0, g.width, g.height);
}

void MetalAPI::endFrame() {
	if (pimpl->encoder == nil) return;
	[pimpl->encoder endEncoding];
	if (pimpl->drawable != nil) {
		[pimpl->cmdBuffer presentDrawable:pimpl->drawable];
	}
	// buffers released during this frame become reusable once the GPU is
	// done with it
	auto released = MetalBufferPool::instance().takePending();
	if (!released.empty()) {
		auto batch = std::make_shared<std::vector<id<MTLBuffer>>>(std::move(released));
		[pimpl->cmdBuffer addCompletedHandler:^(id<MTLCommandBuffer>) {
			MetalBufferPool::instance().recycle(std::move(*batch));
		}];
	}
	[pimpl->cmdBuffer commit];
	pimpl->encoder	 = nil;
	pimpl->cmdBuffer = nil;
	pimpl->drawable	 = nil;
}

bool MetalAPI::inFrame() const {
	return pimpl->encoder != nil;
}

void MetalAPI::maskOn(const Rectf &r) {
	maskIsOn = true;
	maskRect = r;
	if (pimpl->encoder == nil) return;
	// UI coordinates are already in framebuffer pixels (same as the Sokol
	// backend, see SokolAPI::maskOn). Metal validation requires the scissor
	// to stay inside the render target, so intersect the rect with the
	// framebuffer (clamping both edges, like sokol's _sg_clipi).
	const int x0 = std::max(0, (int) r.x);
	const int y0 = std::max(0, (int) r.y);
	const int x1 = std::min((int) (r.x + r.width), pimpl->fbWidth);
	const int y1 = std::min((int) (r.y + r.height), pimpl->fbHeight);
	int x		 = x0;
	int y		 = y0;
	int w		 = x1 - x0;
	int h		 = y1 - y0;
	if (w <= 0 || h <= 0) {
		w = 0;
		h = 0;
		x = 0;
		y = 0;
	}
	[pimpl->encoder setScissorRect:(MTLScissorRect) {(NSUInteger) x, (NSUInteger) y, (NSUInteger) w, (NSUInteger) h}];
}

void MetalAPI::maskOff() {
	maskRect = Rectf(0, 0, g.width, g.height);
	maskIsOn = false;
	if (pimpl->encoder == nil) return;
	[pimpl->encoder
		setScissorRect:(MTLScissorRect) {0, 0, (NSUInteger) pimpl->fbWidth, (NSUInteger) pimpl->fbHeight}];
}

void MetalAPI::clear(vec4 c) {
	// same scheme as the Sokol backend: draw a full-screen quad with blending
	// off, restoring the caller's blend state afterwards
	auto vbo			   = Vbo::create();
	const bool wasBlending = g.isBlending();
	g.setBlending(false);
	g.setColor(c.r, c.g, c.b, c.a);
	vbo->setVertices({{0, 0}, {g.width, 0}, {g.width, g.height}, {0, g.height}});
	vbo->setIndices({0, 1, 2, 0, 2, 3});
	vbo->draw(g);
	g.setBlending(wasBlending);
}

void MetalAPI::drawVerts(const std::vector<glm::vec2> &verts, Vbo::PrimitiveType type) {
	auto vbo = Vbo::create();
	vbo->setVertices(verts);
	vbo->draw(g, type);
}
