#ifdef _WIN32
#include "D3D11Context.h"
#include "log.h"
#include <cstring>

D3D11Context::~D3D11Context() {
	shutdown();
}

bool D3D11Context::init(HWND hwnd, int width, int height, int sampleCount) {
	this->hwnd		  = hwnd;
	this->width		  = width;
	this->height	  = height;
	this->sampleCount = sampleCount;

	DXGI_SWAP_CHAIN_DESC swapChainDesc;
	memset(&swapChainDesc, 0, sizeof(swapChainDesc));
	swapChainDesc.BufferCount						 = 2;
	swapChainDesc.BufferDesc.Width					 = width;
	swapChainDesc.BufferDesc.Height					 = height;
	swapChainDesc.BufferDesc.Format					 = DXGI_FORMAT_B8G8R8A8_UNORM;
	swapChainDesc.BufferDesc.RefreshRate.Numerator	 = 60;
	swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
	swapChainDesc.BufferUsage						 = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.OutputWindow						 = hwnd;
	swapChainDesc.SampleDesc.Count					 = 1; // swap chain is always non-MSAA
	swapChainDesc.SampleDesc.Quality				 = 0;
	swapChainDesc.Windowed							 = TRUE;
	swapChainDesc.SwapEffect						 = DXGI_SWAP_EFFECT_DISCARD;
	swapChainDesc.Flags								 = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	D3D_FEATURE_LEVEL featureLevel;
	D3D_FEATURE_LEVEL featureLevels[] = {D3D_FEATURE_LEVEL_11_0};

	UINT createDeviceFlags = 0;

	HRESULT hr = D3D11CreateDeviceAndSwapChain(nullptr,
											   D3D_DRIVER_TYPE_HARDWARE,
											   nullptr,
											   createDeviceFlags,
											   featureLevels,
											   1,
											   D3D11_SDK_VERSION,
											   &swapChainDesc,
											   &swapChain,
											   &device,
											   &featureLevel,
											   &deviceContext);

	if (FAILED(hr)) {
		Log::e() << "Failed to create D3D11 device and swap chain: 0x" << std::hex << hr;
		return false;
	}

	Log::d() << "D3D11 device created, feature level: " << std::hex << featureLevel;
	createRenderTarget();
	return true;
}

void D3D11Context::createRenderTarget() {
	// Get back buffer and create non-MSAA render target view
	ID3D11Texture2D *backBuffer = nullptr;
	swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void **) &backBuffer);
	device->CreateRenderTargetView(backBuffer, nullptr, &renderTargetView);
	backBuffer->Release();

	if (sampleCount > 1) {
		// Create MSAA color texture
		D3D11_TEXTURE2D_DESC msaaDesc;
		memset(&msaaDesc, 0, sizeof(msaaDesc));
		msaaDesc.Width				= width;
		msaaDesc.Height				= height;
		msaaDesc.MipLevels			= 1;
		msaaDesc.ArraySize			= 1;
		msaaDesc.Format				= DXGI_FORMAT_B8G8R8A8_UNORM;
		msaaDesc.SampleDesc.Count	= sampleCount;
		msaaDesc.SampleDesc.Quality = 0;
		msaaDesc.Usage				= D3D11_USAGE_DEFAULT;
		msaaDesc.BindFlags			= D3D11_BIND_RENDER_TARGET;

		device->CreateTexture2D(&msaaDesc, nullptr, &msaaColorTexture);
		device->CreateRenderTargetView(msaaColorTexture, nullptr, &msaaRenderTargetView);
	}
}

void D3D11Context::releaseRenderTarget() {
	if (msaaRenderTargetView) {
		msaaRenderTargetView->Release();
		msaaRenderTargetView = nullptr;
	}
	if (msaaColorTexture) {
		msaaColorTexture->Release();
		msaaColorTexture = nullptr;
	}
	if (depthStencilView) {
		depthStencilView->Release();
		depthStencilView = nullptr;
	}
	if (depthStencilTexture) {
		depthStencilTexture->Release();
		depthStencilTexture = nullptr;
	}
	if (renderTargetView) {
		renderTargetView->Release();
		renderTargetView = nullptr;
	}
}

void D3D11Context::resize(int newWidth, int newHeight) {
	if (newWidth == 0 || newHeight == 0) return;
	width  = newWidth;
	height = newHeight;

	releaseRenderTarget();

	swapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);

	createRenderTarget();
}

void D3D11Context::present() {
	swapChain->Present(1, 0); // vsync on
}

void D3D11Context::shutdown() {
	releaseRenderTarget();
	if (swapChain) {
		swapChain->Release();
		swapChain = nullptr;
	}
	if (deviceContext) {
		deviceContext->Release();
		deviceContext = nullptr;
	}
	if (device) {
		device->Release();
		device = nullptr;
	}
}

#endif // _WIN32
