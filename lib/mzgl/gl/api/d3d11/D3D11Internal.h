#pragma once

// INTERNAL to the D3D11 backend - only include from the api/d3d11/*.cpp files
// and the two Windows views (GLFWAppRunner / MzglVST3View_win). Holds the COM
// side that the pure-C++ backend headers hide: the process-global device
// registry (device + immediate context, the shared fixed-function states, the
// sampler cache, the stream ring and the texture registry) plus the
// immediate-context state memos.
//
// Everything is per device. A module (the standalone exe, or a plugin DLL
// whose editors share one device) has exactly one current device; swapping it
// (device lost) bumps `generation` and every cached resource recreates itself
// lazily from its CPU copy, or - for textures, which keep none - drops out.

#ifndef _WIN32
#	error "D3D11Internal.h is Windows only"
#endif

#include <d3d11.h>
#include <dxgi.h>
#include <cstdint>
#include <mutex>
#include <unordered_map>
#include <vector>
#include "Graphics.h"

// release-and-null a COM pointer (null-safe)
template <typename T>
inline void mzglD3D11Release(T *&p) {
	if (p != nullptr) {
		p->Release();
		p = nullptr;
	}
}

// log a failed HRESULT with some context; returns SUCCEEDED(hr)
bool mzglD3D11Check(HRESULT hr, const char *what);

namespace mzglD3D11 {

	// Stream ring for per-draw geometry: drawVerts, pooled Vbos, the fontstash
	// and the first draw of any freshly set Vbo. One DYNAMIC buffer, appended
	// with MAP_WRITE_NO_OVERWRITE and renamed with MAP_WRITE_DISCARD when it
	// runs out - the driver hands out fresh memory on DISCARD, so draws already
	// queued keep reading the old pages. `epoch` counts DISCARDs: streamed
	// data stays bindable at its offset until the epoch changes.
	struct StreamRing {
		ID3D11Buffer *buffer = nullptr;
		uint32_t capacity	 = 0;
		uint32_t head		 = 0;
		uint32_t epoch		 = 1;
	};

	struct TextureEntry {
		ID3D11Texture2D *texture	  = nullptr;
		ID3D11ShaderResourceView *srv = nullptr;
		int width					  = 0;
		int height					  = 0;
		DXGI_FORMAT format			  = DXGI_FORMAT_UNKNOWN;
	};

	// immediate-context state memos, shared by every Graphics rendering on the
	// device and reset each beginFrame: the driver's state-change cost per
	// draw is measurable and UI draws come in long runs of the same state
	struct ContextMemo {
		ID3D11VertexShader *vs			  = nullptr;
		ID3D11PixelShader *ps			  = nullptr;
		ID3D11InputLayout *layout		  = nullptr;
		ID3D11BlendState *blend			  = nullptr;
		ID3D11ShaderResourceView *srv	  = nullptr;
		uint32_t texId					  = 0; // registry id behind `srv`
		ID3D11SamplerState *sampler		  = nullptr;
		ID3D11Buffer *vsConstants		  = nullptr;
		ID3D11Buffer *psConstants		  = nullptr;
		D3D11_PRIMITIVE_TOPOLOGY topology = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
	};

	struct DeviceState {
		std::mutex mut;
		ID3D11Device *device		 = nullptr;
		ID3D11DeviceContext *context = nullptr;
		uint32_t generation			 = 0;

		ID3D11RasterizerState *rasterizer	  = nullptr;
		ID3D11DepthStencilState *depthStencil = nullptr;
		ID3D11BlendState *blendOff			  = nullptr;
		ID3D11BlendState *blendAlpha		  = nullptr;
		ID3D11BlendState *blendAdditive		  = nullptr;
		// blending on with a mode sokol has no factors for: its defaults, ONE/ZERO
		ID3D11BlendState *blendOpaque = nullptr;

		std::unordered_map<uint32_t, ID3D11SamplerState *> samplers;
		StreamRing ring;

		std::unordered_map<uint32_t, TextureEntry> textures;
		uint32_t nextTextureId = 1;

		ContextMemo memo;

		// the debug layer's message queue when the device has one (DEBUG
		// builds, or MZGL_D3D11_DEBUG=1), drained into the log each frame
		ID3D11InfoQueue *infoQueue = nullptr;

		// MZGL_D3D11_TRACE=<ms>: log every draw of the first frame that starts
		// more than <ms> after the device was adopted (a one-frame draw trace
		// for diagnosing what a screen is made of)
		bool traceThisFrame			 = false;
		bool traced					 = false;
		unsigned long long traceAtMs = 0;

		// live-resource counters for the perf/memory report
		int liveBuffers	 = 0;
		int liveTextures = 0;
	};

	// deliberately leaked: Vbo/texture/shader destructors run at exit time and
	// must not touch a destroyed mutex
	DeviceState &state();

	// Adopt a device (AddRef'd) for everything created from now on. A device
	// different from the current one invalidates every cached resource. Null
	// releases the device.
	void setDevice(ID3D11Device *device);
	ID3D11Device *device();
	ID3D11DeviceContext *context();
	uint32_t generation();

	// sampler for a D3D11Texture::samplerKey() (bit0 linear, bit1 wrap x, bit2 wrap y)
	ID3D11SamplerState *sampler(uint32_t key);
	ID3D11BlendState *blendState(bool enabled, Graphics::BlendMode mode);

	// Append `size` bytes to the stream ring; outOffset is 16-byte aligned.
	// Returns false when size exceeds the ring (caller makes a one-shot buffer).
	bool streamAppend(const void *data, uint32_t size, uint32_t &outOffset, uint32_t &outEpoch);
	ID3D11Buffer *streamBuffer();
	uint32_t streamEpoch();
	// Make room for `bytes` of appends BEFORE a draw streams its buffers: a
	// wrap (DISCARD, new memory) between two appends of one draw would leave
	// the earlier binding pointing at the renamed buffer. Wraps now so the
	// draw's appends can't.
	void streamReserve(uint32_t bytes);
	// ~2MB: the busiest screen streams a few hundred KB per frame, and a
	// DISCARD on wrap is cheap anyway
	constexpr uint32_t kStreamRingBytes = 2 * 1024 * 1024;

	// texture registry: uint32_t handle -> texture + SRV (mirrors sokol's image
	// handles so Texture::create(g, id, w, h) keeps working for the font atlas)
	uint32_t registerTexture(const TextureEntry &entry); // takes ownership of the COM refs
	void unregisterTexture(uint32_t id);
	// null when unknown, or when the device was replaced since creation
	ID3D11ShaderResourceView *lookupTexture(uint32_t id);
	bool lookupTextureEntry(uint32_t id, TextureEntry &out);
	bool textureValid(uint32_t id);

	// bind the api.s bound texture + sampler to the pixel stage (memoised);
	// defined in D3D11Vbo.cpp
	void applyTextureBinding(class D3D11API &api, bool hasTexCoords);

	// set the context's primitive topology, skipping the call when unchanged
	void setTopology(D3D11_PRIMITIVE_TOPOLOGY t);

	// forward queued D3D11 debug-layer messages to the log (no-op without the layer)
	void drainDebugMessages();
} // namespace mzglD3D11
