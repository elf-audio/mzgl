#include "D3D11API.h"
#include "D3D11Internal.h"
#include "D3D11Context.h"
#include "D3D11Vbo.h"
#include "D3D11Shader.h"
#include "Graphics.h"
#include "Image.h"
#include "SokolDefaultShaders.h"
#include <ios>
#include <cstdlib>

bool mzglD3D11Check(HRESULT hr, const char *what) {
	if (SUCCEEDED(hr)) return true;
	Log::e() << "D3D11: " << what << " failed: 0x" << std::hex << (unsigned long) hr << std::dec;
	return false;
}

// ---------------------------------------------------------------------------
// process-global device registry
// ---------------------------------------------------------------------------
namespace mzglD3D11 {

	DeviceState &state() {
		static DeviceState *s = new DeviceState();
		return *s;
	}

	namespace {
		ID3D11BlendState *makeBlend(ID3D11Device *dev, bool enabled, D3D11_BLEND src, D3D11_BLEND dst) {
			D3D11_BLEND_DESC d		 = {};
			d.AlphaToCoverageEnable	 = FALSE;
			d.IndependentBlendEnable = FALSE;
			auto &rt				 = d.RenderTarget[0];
			rt.BlendEnable			 = enabled ? TRUE : FALSE;
			rt.SrcBlend				 = src;
			rt.DestBlend			 = dst;
			rt.BlendOp				 = D3D11_BLEND_OP_ADD;
			// alpha factors are sokol's defaults (src ONE, dst ZERO), the same
			// as the Sokol and Metal backends
			rt.SrcBlendAlpha		 = D3D11_BLEND_ONE;
			rt.DestBlendAlpha		 = D3D11_BLEND_ZERO;
			rt.BlendOpAlpha			 = D3D11_BLEND_OP_ADD;
			rt.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
			ID3D11BlendState *bs	 = nullptr;
			mzglD3D11Check(dev->CreateBlendState(&d, &bs), "CreateBlendState");
			return bs;
		}

		// caller holds the lock
		void createDeviceResources(DeviceState &s) {
			ID3D11Device *dev = s.device;

			D3D11_RASTERIZER_DESC rs = {};
			rs.FillMode				 = D3D11_FILL_SOLID;
			rs.CullMode				 = D3D11_CULL_NONE;
			rs.FrontCounterClockwise = FALSE;
			rs.DepthClipEnable		 = TRUE;
			// masks are a dynamic scissor rect (see D3D11API::maskOn), never a
			// pipeline permutation
			rs.ScissorEnable		 = TRUE;
			rs.MultisampleEnable	 = mzglD3D11SampleCount > 1 ? TRUE : FALSE;
			rs.AntialiasedLineEnable = FALSE;
			mzglD3D11Check(dev->CreateRasterizerState(&rs, &s.rasterizer), "CreateRasterizerState");

			D3D11_DEPTH_STENCIL_DESC ds = {};
			ds.DepthEnable				= FALSE;
			ds.DepthWriteMask			= D3D11_DEPTH_WRITE_MASK_ZERO;
			ds.DepthFunc				= D3D11_COMPARISON_ALWAYS;
			ds.StencilEnable			= FALSE;
			mzglD3D11Check(dev->CreateDepthStencilState(&ds, &s.depthStencil), "CreateDepthStencilState");

			s.blendOff		= makeBlend(dev, false, D3D11_BLEND_ONE, D3D11_BLEND_ZERO);
			s.blendAlpha	= makeBlend(dev, true, D3D11_BLEND_SRC_ALPHA, D3D11_BLEND_INV_SRC_ALPHA);
			s.blendAdditive = makeBlend(dev, true, D3D11_BLEND_ONE, D3D11_BLEND_ONE);
			s.blendOpaque	= makeBlend(dev, true, D3D11_BLEND_ONE, D3D11_BLEND_ZERO);

			D3D11_BUFFER_DESC bd = {};
			bd.ByteWidth		 = kStreamRingBytes;
			bd.Usage			 = D3D11_USAGE_DYNAMIC;
			bd.BindFlags		 = D3D11_BIND_VERTEX_BUFFER | D3D11_BIND_INDEX_BUFFER;
			bd.CPUAccessFlags	 = D3D11_CPU_ACCESS_WRITE;
			if (mzglD3D11Check(dev->CreateBuffer(&bd, nullptr, &s.ring.buffer), "CreateBuffer(stream ring)")) {
				s.ring.capacity = kStreamRingBytes;
				s.liveBuffers++;
			}
			s.ring.head = 0;
			s.ring.epoch++;

			if (FAILED(dev->QueryInterface(__uuidof(ID3D11InfoQueue), (void **) &s.infoQueue))) {
				s.infoQueue = nullptr;
			}
		}

		// caller holds the lock
		void releaseDeviceResources(DeviceState &s) {
			mzglD3D11Release(s.rasterizer);
			mzglD3D11Release(s.depthStencil);
			mzglD3D11Release(s.blendOff);
			mzglD3D11Release(s.blendAlpha);
			mzglD3D11Release(s.blendAdditive);
			mzglD3D11Release(s.blendOpaque);
			for (auto &kv: s.samplers)
				mzglD3D11Release(kv.second);
			s.samplers.clear();
			mzglD3D11Release(s.infoQueue);
			if (s.ring.buffer != nullptr) s.liveBuffers--;
			mzglD3D11Release(s.ring.buffer);
			s.ring.capacity = 0;
			s.ring.head		= 0;
			s.ring.epoch++;
			// textures belong to the dead device and have no CPU copy: drop the
			// COM objects but keep the ids registered so their owners can still
			// unregister them (lookups return null until then)
			for (auto &kv: s.textures) {
				if (kv.second.srv != nullptr) s.liveTextures--;
				mzglD3D11Release(kv.second.srv);
				mzglD3D11Release(kv.second.texture);
			}
			s.memo = ContextMemo();
		}
	} // namespace

	void setDevice(ID3D11Device *dev) {
		auto &s = state();
		std::lock_guard<std::mutex> l(s.mut);
		if (dev == s.device) return;
		if (s.device != nullptr) {
			Log::d() << "D3D11: replacing device (generation " << s.generation << ")";
		}
		releaseDeviceResources(s);
		mzglD3D11Release(s.context);
		mzglD3D11Release(s.device);
		s.generation++;
		if (dev == nullptr) return;
		s.device = dev;
		s.device->AddRef();
		s.device->GetImmediateContext(&s.context);
		createDeviceResources(s);
		if (const char *env = getenv("MZGL_D3D11_TRACE"); env != nullptr && atoi(env) > 0) {
			s.traceAtMs = GetTickCount64() + (unsigned long long) atoi(env);
			s.traced	= false;
		}
	}

	ID3D11Device *device() {
		return state().device;
	}
	ID3D11DeviceContext *context() {
		return state().context;
	}
	uint32_t generation() {
		return state().generation;
	}

	ID3D11SamplerState *sampler(uint32_t key) {
		auto &s = state();
		std::lock_guard<std::mutex> l(s.mut);
		auto it = s.samplers.find(key);
		if (it != s.samplers.end()) return it->second;
		if (s.device == nullptr) return nullptr;

		D3D11_SAMPLER_DESC d	= {};
		const bool linear		= (key & 1u) != 0;
		d.Filter				= linear ? D3D11_FILTER_MIN_MAG_MIP_LINEAR : D3D11_FILTER_MIN_MAG_MIP_POINT;
		d.AddressU				= (key & 2u) ? D3D11_TEXTURE_ADDRESS_WRAP : D3D11_TEXTURE_ADDRESS_CLAMP;
		d.AddressV				= (key & 4u) ? D3D11_TEXTURE_ADDRESS_WRAP : D3D11_TEXTURE_ADDRESS_CLAMP;
		d.AddressW				= D3D11_TEXTURE_ADDRESS_CLAMP;
		d.MaxAnisotropy			= 1;
		d.ComparisonFunc		= D3D11_COMPARISON_NEVER;
		d.MinLOD				= 0.f;
		d.MaxLOD				= D3D11_FLOAT32_MAX;
		ID3D11SamplerState *smp = nullptr;
		if (!mzglD3D11Check(s.device->CreateSamplerState(&d, &smp), "CreateSamplerState")) return nullptr;
		s.samplers[key] = smp;
		return smp;
	}

	ID3D11BlendState *blendState(bool enabled, Graphics::BlendMode mode) {
		auto &s = state();
		if (!enabled) return s.blendOff;
		switch (mode) {
			case Graphics::BlendMode::Alpha: return s.blendAlpha;
			case Graphics::BlendMode::Additive: return s.blendAdditive;
			default: return s.blendOpaque;
		}
	}

	bool streamAppend(const void *data, uint32_t size, uint32_t &outOffset, uint32_t &outEpoch) {
		auto &s	   = state();
		auto &ring = s.ring;
		if (ring.buffer == nullptr || s.context == nullptr || size > ring.capacity) return false;
		uint32_t head  = (ring.head + 15u) & ~15u;
		D3D11_MAP mode = D3D11_MAP_WRITE_NO_OVERWRITE;
		if (head + size > ring.capacity) {
			mode = D3D11_MAP_WRITE_DISCARD;
			head = 0;
			ring.epoch++;
		}
		D3D11_MAPPED_SUBRESOURCE mapped = {};
		if (!mzglD3D11Check(s.context->Map(ring.buffer, 0, mode, 0, &mapped), "Map(stream ring)")) {
			return false;
		}
		memcpy(static_cast<uint8_t *>(mapped.pData) + head, data, size);
		s.context->Unmap(ring.buffer, 0);
		outOffset = head;
		outEpoch  = ring.epoch;
		ring.head = head + size;
		return true;
	}

	void streamReserve(uint32_t bytes) {
		auto &s	   = state();
		auto &ring = s.ring;
		if (ring.buffer == nullptr || s.context == nullptr || bytes > ring.capacity) return;
		const uint32_t head = (ring.head + 15u) & ~15u;
		if (head + bytes <= ring.capacity) return;
		D3D11_MAPPED_SUBRESOURCE mapped = {};
		if (!mzglD3D11Check(s.context->Map(ring.buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped),
							"Map(stream ring, reserve)")) {
			return;
		}
		s.context->Unmap(ring.buffer, 0);
		ring.head = 0;
		ring.epoch++;
	}

	ID3D11Buffer *streamBuffer() {
		return state().ring.buffer;
	}
	uint32_t streamEpoch() {
		return state().ring.epoch;
	}

	uint32_t registerTexture(const TextureEntry &entry) {
		auto &s = state();
		std::lock_guard<std::mutex> l(s.mut);
		const uint32_t id = s.nextTextureId++;
		s.textures[id]	  = entry;
		s.liveTextures++;
		return id;
	}

	void unregisterTexture(uint32_t id) {
		auto &s = state();
		std::lock_guard<std::mutex> l(s.mut);
		auto it = s.textures.find(id);
		if (it == s.textures.end()) return;
		if (it->second.srv != nullptr) s.liveTextures--;
		mzglD3D11Release(it->second.srv);
		mzglD3D11Release(it->second.texture);
		s.textures.erase(it);
	}

	ID3D11ShaderResourceView *lookupTexture(uint32_t id) {
		auto &s = state();
		std::lock_guard<std::mutex> l(s.mut);
		auto it = s.textures.find(id);
		return it == s.textures.end() ? nullptr : it->second.srv;
	}

	bool lookupTextureEntry(uint32_t id, TextureEntry &out) {
		auto &s = state();
		std::lock_guard<std::mutex> l(s.mut);
		auto it = s.textures.find(id);
		if (it == s.textures.end() || it->second.srv == nullptr) return false;
		out = it->second;
		return true;
	}

	bool textureValid(uint32_t id) {
		return lookupTexture(id) != nullptr;
	}

	void setTopology(D3D11_PRIMITIVE_TOPOLOGY t) {
		auto &s = state();
		if (s.memo.topology == t) return;
		s.memo.topology = t;
		s.context->IASetPrimitiveTopology(t);
	}

	void drainDebugMessages() {
		auto &s				   = state();
		ID3D11InfoQueue *queue = s.infoQueue;
		if (queue == nullptr) return;
		const UINT64 n = queue->GetNumStoredMessages();
		if (n == 0) return;
		std::vector<char> buf;
		for (UINT64 i = 0; i < n; i++) {
			SIZE_T len = 0;
			if (FAILED(queue->GetMessage(i, nullptr, &len)) || len == 0) continue;
			buf.resize(len);
			auto *msg = reinterpret_cast<D3D11_MESSAGE *>(buf.data());
			if (FAILED(queue->GetMessage(i, msg, &len))) continue;
			if (msg->Severity == D3D11_MESSAGE_SEVERITY_INFO || msg->Severity == D3D11_MESSAGE_SEVERITY_MESSAGE) {
				continue;
			}
			Log::e() << "D3D11 debug layer: " << std::string(msg->pDescription, msg->DescriptionByteLength);
		}
		queue->ClearStoredMessages();
	}
} // namespace mzglD3D11

// ---------------------------------------------------------------------------
// D3D11API
// ---------------------------------------------------------------------------
struct D3D11APIImpl {
	// set by beginFrame; stays for readback after endFrame (the context
	// outlives every frame)
	D3D11Context *ctx = nullptr;
	int fbWidth		  = 0;
	int fbHeight	  = 0;
	bool frameOpen	  = false;
	std::string pendingScreenshot;

	// scratch Vbos for the immediate-mode draws (clear, drawVerts): streamed
	// each call, so no GPU buffer churn and no per-call heap object
	VboRef scratchVbo;
	VboRef scratchColorVbo;
};

D3D11API::D3D11API(Graphics &g)
	: GraphicsAPI(g)
	, pimpl(std::make_unique<D3D11APIImpl>()) {
}

D3D11API::~D3D11API() = default;

void D3D11API::init() {
	loadDefaultShaders();
	// Screenshots: flag the request and capture the finished frame in
	// endFrame, after the MSAA resolve and before present (the same scheme as
	// the Metal view). The Graphics owns both this lambda and the api, so the
	// plain pointer can't dangle.
	D3D11API *self		 = this;
	g.deferredSaveScreen = [self](const std::string &path) -> bool {
		self->pimpl->pendingScreenshot = path;
		return true;
	};
}

void D3D11API::loadDefaultShaders() {
	registerShaders(shaderRegistry);
	g.nothingShader		 = createDefaultShader("nothing", &g);
	g.colorShader		 = createDefaultShader("color", &g);
	g.colorTextureShader = createDefaultShader("colorTexture", &g);
	g.texShader			 = createDefaultShader("tex", &g);
	g.fontShader		 = createDefaultShader("font", &g);
	g.colorFontShader	 = createDefaultShader("colorFont", &g);
}

// the shader registry of the active backend, for generated shader-registration
// code (customShaders.h) that must work on every compiled-shader backend
SokolShaderRegistry &mzglShaderRegistry(Graphics &g) {
	return static_cast<D3D11API &>(g.getAPI()).getShaderRegistry();
}

void D3D11API::beginFrame(D3D11Context &ctx) {
	auto &s = mzglD3D11::state();
	if (s.device == nullptr || s.context == nullptr) {
		Log::e() << "D3D11API::beginFrame: no device - call mzglD3D11::setDevice first";
		return;
	}
	if (ctx.getDevice() != s.device) {
		Log::e() << "D3D11API::beginFrame: the context's device is not the current backend device";
		return;
	}
	ID3D11RenderTargetView *rtv =
		ctx.getSampleCount() > 1 ? ctx.getMSAARenderTargetView() : ctx.getRenderTargetView();
	if (rtv == nullptr) return;

	pimpl->ctx		= &ctx;
	pimpl->fbWidth	= ctx.getWidth();
	pimpl->fbHeight = ctx.getHeight();

	auto *dc = s.context;
	dc->OMSetRenderTargets(1, &rtv, nullptr);
	const float black[4] = {0.f, 0.f, 0.f, 1.f};
	dc->ClearRenderTargetView(rtv, black);

	D3D11_VIEWPORT vp = {};
	vp.Width		  = (float) pimpl->fbWidth;
	vp.Height		  = (float) pimpl->fbHeight;
	vp.MinDepth		  = 0.f;
	vp.MaxDepth		  = 1.f;
	dc->RSSetViewports(1, &vp);
	dc->RSSetState(s.rasterizer);
	dc->OMSetDepthStencilState(s.depthStencil, 0);
	s.memo = mzglD3D11::ContextMemo();

	pimpl->frameOpen = true;
	maskOff();

	if (s.traceAtMs != 0 && !s.traced && GetTickCount64() >= s.traceAtMs) {
		s.traced		 = true;
		s.traceThisFrame = true;
		Log::d() << "[D3D11 trace] frame " << g.getFrameNum() << " begin " << pimpl->fbWidth << "x"
				 << pimpl->fbHeight;
	}
}

bool D3D11API::endFrame(bool vsync) {
	if (!pimpl->frameOpen) return true;
	pimpl->frameOpen = false;
	auto &s			 = mzglD3D11::state();
	if (s.traceThisFrame) {
		s.traceThisFrame = false;
		Log::d() << "[D3D11 trace] frame end";
	}
	auto *dc  = s.context;
	auto &ctx = *pimpl->ctx;

	if (ctx.getSampleCount() > 1 && ctx.getMSAAColorTexture() != nullptr
		&& ctx.getBackBufferTexture() != nullptr) {
		dc->ResolveSubresource(
			ctx.getBackBufferTexture(), 0, ctx.getMSAAColorTexture(), 0, DXGI_FORMAT_B8G8R8A8_UNORM);
	}

	if (!pimpl->pendingScreenshot.empty()) {
		std::string path = std::move(pimpl->pendingScreenshot);
		pimpl->pendingScreenshot.clear();
		std::vector<uint8_t> rgba;
		if (ctx.readBackBuffer(rgba)) {
			ImageRef img = Image::create(ctx.getWidth(), ctx.getHeight(), 4);
			img->data	 = std::move(rgba);
			img->save(path);
		} else {
			Log::e() << "D3D11API: screenshot readback failed for " << path;
		}
	}

	// unbind the render target (sokol's end-pass does the same): a bound view
	// would keep the swap chain from resizing
	ID3D11RenderTargetView *nullRtv = nullptr;
	dc->OMSetRenderTargets(1, &nullRtv, nullptr);

	const HRESULT hr = ctx.present(vsync);
	mzglD3D11::drainDebugMessages();
	if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET) {
		const HRESULT reason = s.device->GetDeviceRemovedReason();
		Log::e() << "D3D11: device lost on Present (0x" << std::hex << (unsigned long) hr << ", reason 0x"
				 << (unsigned long) reason << std::dec << ") - recreating";
		return false;
	}
	if (FAILED(hr)) {
		mzglD3D11Check(hr, "Present");
	}
	return true;
}

bool D3D11API::inFrame() const {
	return pimpl->frameOpen;
}

void D3D11API::maskOn(const Rectf &r) {
	maskIsOn = true;
	maskRect = r;
	if (!pimpl->frameOpen) return;
	// UI coordinates are already in framebuffer pixels (same as the Sokol
	// backend, see SokolAPI::maskOn). Clamp to the render target like sokol's
	// _sg_clipi does.
	const int x0 = std::max(0, (int) r.x);
	const int y0 = std::max(0, (int) r.y);
	const int x1 = std::min((int) (r.x + r.width), pimpl->fbWidth);
	const int y1 = std::min((int) (r.y + r.height), pimpl->fbHeight);
	D3D11_RECT rect;
	if (x1 <= x0 || y1 <= y0) {
		rect = {0, 0, 0, 0};
	} else {
		rect = {x0, y0, x1, y1};
	}
	mzglD3D11::context()->RSSetScissorRects(1, &rect);
}

void D3D11API::maskOff() {
	maskRect = Rectf(0, 0, g.width, g.height);
	maskIsOn = false;
	if (!pimpl->frameOpen) return;
	D3D11_RECT rect = {0, 0, pimpl->fbWidth, pimpl->fbHeight};
	mzglD3D11::context()->RSSetScissorRects(1, &rect);
}

void D3D11API::clear(vec4 c) {
	// same scheme as the Sokol backend: draw a full-screen quad with blending
	// off, restoring the caller's blend state afterwards
	if (pimpl->scratchVbo == nullptr) pimpl->scratchVbo = Vbo::createFromPool(g);
	auto &vbo			   = *pimpl->scratchVbo;
	const bool wasBlending = g.isBlending();
	g.setBlending(false);
	g.setColor(c.r, c.g, c.b, c.a);
	vbo.deallocate();
	vbo.setVertices({{0, 0}, {g.width, 0}, {g.width, g.height}, {0, g.height}});
	vbo.setIndices({0, 1, 2, 0, 2, 3});
	// the scratch Vbo is shared with drawVerts, and Vbo::draw remembers the
	// last explicit primitive type - so always say triangles here
	vbo.draw(g, Vbo::PrimitiveType::Triangles);
	g.setBlending(wasBlending);
}

void D3D11API::drawVerts(const std::vector<glm::vec2> &verts, Vbo::PrimitiveType type) {
	if (pimpl->scratchVbo == nullptr) pimpl->scratchVbo = Vbo::createFromPool(g);
	auto &vbo = *pimpl->scratchVbo;
	vbo.deallocate();
	vbo.setVertices(verts);
	vbo.draw(g, type);
}

void D3D11API::drawVerts(const std::vector<glm::vec2> &verts,
						 const std::vector<glm::vec4> &cols,
						 Vbo::PrimitiveType type) {
	if (pimpl->scratchColorVbo == nullptr) pimpl->scratchColorVbo = Vbo::createFromPool(g);
	auto &vbo = *pimpl->scratchColorVbo;
	vbo.deallocate();
	vbo.setVertices(verts);
	vbo.setColors(cols);
	vbo.draw(g, type);
}

void D3D11API::readScreenPixels(std::vector<uint8_t> &outData, const Rectf &r) {
	outData.clear();
	auto *ctx = pimpl->ctx;
	if (ctx == nullptr || mzglD3D11::device() == nullptr) return;
	// what's been drawn so far lives in the MSAA target mid-frame; resolve it
	// into a scratch texture (the back buffer only gets the resolve at endFrame)
	ID3D11Texture2D *src	 = ctx->getBackBufferTexture();
	ID3D11Texture2D *scratch = nullptr;
	if (ctx->getSampleCount() > 1 && ctx->getMSAAColorTexture() != nullptr) {
		D3D11_TEXTURE2D_DESC d = {};
		ctx->getMSAAColorTexture()->GetDesc(&d);
		d.SampleDesc.Count	 = 1;
		d.SampleDesc.Quality = 0;
		d.BindFlags			 = 0;
		d.Usage				 = D3D11_USAGE_DEFAULT;
		if (!mzglD3D11Check(mzglD3D11::device()->CreateTexture2D(&d, nullptr, &scratch),
							"CreateTexture2D(readback resolve)")) {
			return;
		}
		mzglD3D11::context()->ResolveSubresource(scratch, 0, ctx->getMSAAColorTexture(), 0, d.Format);
		src = scratch;
	}
	if (src == nullptr) return;
	std::vector<uint8_t> topDown;
	const int x = std::max(0, (int) r.x);
	const int y = std::max(0, (int) r.y);
	const int w = std::min((int) r.width, ctx->getWidth() - x);
	const int h = std::min((int) r.height, ctx->getHeight() - y);
	if (w > 0 && h > 0
		&& mzglD3D11ReadTexture(mzglD3D11::device(), mzglD3D11::context(), src, x, y, w, h, topDown)) {
		// glReadPixels order: bottom row first (Graphics::saveScreen flips)
		outData.resize(topDown.size());
		const size_t rowBytes = (size_t) w * 4;
		for (int row = 0; row < h; row++) {
			memcpy(outData.data() + (size_t) (h - 1 - row) * rowBytes,
				   topDown.data() + (size_t) row * rowBytes,
				   rowBytes);
		}
	}
	mzglD3D11Release(scratch);
}

bool mzglD3D11TextureValid(uint32_t id) {
	return mzglD3D11::textureValid(id);
}

void D3D11API::drawTextRun(const float *xyuv, int nverts) {
	if (!pimpl->frameOpen || nverts <= 0) return;
	// mirror of D3D11Vbo::getShader() for a pos+uv-only vertex layout
	D3D11Shader *shader = nullptr;
	if (g.currShader && !g.currShader->isDefaultShader) {
		shader = static_cast<D3D11Shader *>(g.currShader);
	} else if (g.currShader == g.fontShader.get()) {
		shader = static_cast<D3D11Shader *>(g.fontShader.get());
	} else {
		shader = static_cast<D3D11Shader *>(g.texShader.get());
	}
	if (shader == nullptr) return;

	constexpr UINT stride = 4 * sizeof(float);
	uint32_t offset		  = 0;
	uint32_t epoch		  = 0;
	if (!mzglD3D11::streamAppend(xyuv, (uint32_t) nverts * stride, offset, epoch)) return;

	auto *dc		   = mzglD3D11::context();
	ID3D11Buffer *ring = mzglD3D11::streamBuffer();
	const UINT off	   = offset;
	dc->IASetVertexBuffers(0, 1, &ring, &stride, &off);

	SokolVertexAttr attrs[2];
	int numAttrs = 0;
	if (int loc = shader->attrSlot("Position"); loc >= 0) {
		attrs[numAttrs++] = {loc, SG_VERTEXFORMAT_FLOAT2, 0, false, 0, (int) stride};
	}
	if (int loc = shader->attrSlot("TexCoord"); loc >= 0) {
		attrs[numAttrs++] = {loc, SG_VERTEXFORMAT_FLOAT2, 0, false, 8, (int) stride};
	}

	mzglD3D11::applyTextureBinding(*this, true);
	mzglD3D11::setTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	if (!shader->applyPipelineAndUniforms(attrs, numAttrs)) return;
	dc->Draw((UINT) nverts, 0);
}

std::string mzglD3D11ResourceReport() {
	auto &s = mzglD3D11::state();
	std::lock_guard<std::mutex> l(s.mut);
	std::string r = "=== D3D11 backend: " + std::to_string(s.liveBuffers) + " live buffers (incl. stream ring), "
					+ std::to_string(s.liveTextures) + " live textures, " + std::to_string(s.samplers.size())
					+ " samplers, stream ring " + std::to_string(s.ring.head / 1024) + "/"
					+ std::to_string(s.ring.capacity / 1024) + " KB used, epoch " + std::to_string(s.ring.epoch)
					+ ", device generation " + std::to_string(s.generation) + " ===" + std::string(1, (char) 10);
	return r;
}
