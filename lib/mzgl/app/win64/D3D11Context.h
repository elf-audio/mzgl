#pragma once
#ifdef _WIN32

#	include <d3d11.h>
#	include <dxgi.h>
#	include <cstdint>
#	include <vector>

// Copies a w x h region of a (non-multisampled) BGRA8/RGBA8 texture into
// outRgba as top-down RGBA with alpha forced opaque, through a staging copy.
// Used for screenshots by both the D3D11 and the Sokol/D3D11 backends.
bool mzglD3D11ReadTexture(ID3D11Device *device,
						  ID3D11DeviceContext *ctx,
						  ID3D11Texture2D *tex,
						  int x,
						  int y,
						  int w,
						  int h,
						  std::vector<uint8_t> &outRgba);

// One window's D3D11 device + swap chain + render targets. Shared by the
// Sokol/D3D11 backend (which wraps it in sg_environment / sg_pass) and the
// native D3D11 backend (D3D11API::beginFrame/endFrame).
class D3D11Context {
public:
	~D3D11Context();

	// Creates a device + swap chain for hwnd (or an offscreen render target
	// when no swap chain can be made, see isOffscreen). Pass an existing device to share
	// it (and its immediate context) and only create a swap chain for this
	// window - every swap chain a backend renders into must come from the
	// device its resources live on (plugin editors opened several times in
	// one host, say). DEBUG builds try the D3D11 debug layer and fall back to
	// a plain device when the SDK layers aren't installed.
	bool init(HWND hwnd, int width, int height, int sampleCount = 1, ID3D11Device *sharedDevice = nullptr);

	// After a device loss: drop everything and make a fresh device (or adopt
	// sharedDevice) plus a swap chain for the same window and size.
	bool recreate(ID3D11Device *sharedDevice = nullptr);

	// true when no swap chain could be made (no display on the adapter) and
	// frames go to an offscreen texture instead; present() then retries
	bool isOffscreen() const { return offscreen; }

	void shutdown();
	void resize(int width, int height);

	// the Present() result, so callers can spot DXGI_ERROR_DEVICE_REMOVED/RESET
	HRESULT present(bool vsync = true);

	// the (resolved) back buffer as top-down RGBA, alpha opaque
	bool readBackBuffer(std::vector<uint8_t> &outRgba);

	ID3D11Device *getDevice() const { return device; }
	ID3D11DeviceContext *getDeviceContext() const { return deviceContext; }
	IDXGISwapChain *getSwapChain() const { return swapChain; }
	ID3D11RenderTargetView *getRenderTargetView() const { return renderTargetView; }
	ID3D11RenderTargetView *getMSAARenderTargetView() const { return msaaRenderTargetView; }
	ID3D11DepthStencilView *getDepthStencilView() const { return depthStencilView; }
	ID3D11Texture2D *getBackBufferTexture() const { return backBufferTexture; }
	ID3D11Texture2D *getMSAAColorTexture() const { return msaaColorTexture; }

	int getWidth() const { return width; }
	int getHeight() const { return height; }
	int getSampleCount() const { return sampleCount; }
	bool hasDebugLayer() const { return debugLayer; }

private:
	DXGI_SWAP_CHAIN_DESC swapChainDescFor(int w, int h) const;
	bool createSwapChain(const DXGI_SWAP_CHAIN_DESC &desc, bool logFailure = true);
	void tryRestoreSwapChain();
	void createRenderTarget();
	void releaseRenderTarget();

	HWND hwnd						   = nullptr;
	ID3D11Device *device			   = nullptr;
	ID3D11DeviceContext *deviceContext = nullptr;
	IDXGISwapChain *swapChain		   = nullptr;

	// Non-MSAA back buffer render target (used as resolve target when MSAA is on)
	ID3D11Texture2D *backBufferTexture		 = nullptr;
	ID3D11RenderTargetView *renderTargetView = nullptr;

	// MSAA render target (only created when sampleCount > 1)
	ID3D11Texture2D *msaaColorTexture			 = nullptr;
	ID3D11RenderTargetView *msaaRenderTargetView = nullptr;

	// Depth stencil (matches MSAA sample count) - not created at the moment,
	// mzgl renders without depth
	ID3D11Texture2D *depthStencilTexture	 = nullptr;
	ID3D11DepthStencilView *depthStencilView = nullptr;

	int width								= 0;
	int height								= 0;
	int sampleCount							= 1;
	bool debugLayer							= false;
	bool offscreen							= false;
	unsigned long long lastSwapChainRetryMs = 0;
};

#endif // _WIN32
