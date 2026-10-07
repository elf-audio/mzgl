#pragma once
#ifdef _WIN32

#include <d3d11.h>
#include <dxgi.h>

class D3D11Context {
public:
	~D3D11Context();

	// Creates a device + swap chain for hwnd. Pass an existing device to share
	// it (and its immediate context) and only create a swap chain for this
	// window - sokol has exactly one sg_setup() per process, bound to one
	// device, so every swap chain it renders into must come from that device
	// (plugin editors opened several times in one host, say).
	bool init(HWND hwnd, int width, int height, int sampleCount = 1, ID3D11Device *sharedDevice = nullptr);
	void shutdown();
	void resize(int width, int height);
	void present(bool vsync = true);

	ID3D11Device *getDevice() const { return device; }
	ID3D11DeviceContext *getDeviceContext() const { return deviceContext; }
	ID3D11RenderTargetView *getRenderTargetView() const { return renderTargetView; }
	ID3D11RenderTargetView *getMSAARenderTargetView() const { return msaaRenderTargetView; }
	ID3D11DepthStencilView *getDepthStencilView() const { return depthStencilView; }

	int getWidth() const { return width; }
	int getHeight() const { return height; }
	int getSampleCount() const { return sampleCount; }

private:
	bool createSwapChainForSharedDevice(const DXGI_SWAP_CHAIN_DESC &desc);
	void createRenderTarget();
	void releaseRenderTarget();

	HWND hwnd					   = nullptr;
	ID3D11Device *device		   = nullptr;
	ID3D11DeviceContext *deviceContext = nullptr;
	IDXGISwapChain *swapChain	   = nullptr;

	// Non-MSAA back buffer render target (used as resolve target when MSAA is on)
	ID3D11RenderTargetView *renderTargetView = nullptr;

	// MSAA render target (only created when sampleCount > 1)
	ID3D11Texture2D *msaaColorTexture			 = nullptr;
	ID3D11RenderTargetView *msaaRenderTargetView = nullptr;

	// Depth stencil (matches MSAA sample count)
	ID3D11Texture2D *depthStencilTexture	   = nullptr;
	ID3D11DepthStencilView *depthStencilView = nullptr;

	int width		= 0;
	int height		= 0;
	int sampleCount = 1;
};

#endif // _WIN32
