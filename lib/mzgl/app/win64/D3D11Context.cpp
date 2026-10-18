#ifdef _WIN32
#	include "D3D11Context.h"
#	include "log.h"
#	include <cstring>
#	include <cstdlib>
#	include <ios>

namespace {
	void logHr(const char *what, HRESULT hr) {
		Log::e() << "D3D11Context: " << what << " failed: 0x" << std::hex << (unsigned long) hr << std::dec;
	}
} // namespace

bool mzglD3D11ReadTexture(ID3D11Device *device,
						  ID3D11DeviceContext *ctx,
						  ID3D11Texture2D *tex,
						  int x,
						  int y,
						  int w,
						  int h,
						  std::vector<uint8_t> &outRgba) {
	outRgba.clear();
	if (device == nullptr || ctx == nullptr || tex == nullptr || w <= 0 || h <= 0) return false;

	D3D11_TEXTURE2D_DESC src = {};
	tex->GetDesc(&src);
	if (src.SampleDesc.Count > 1) {
		Log::e() << "mzglD3D11ReadTexture: multisampled source - resolve it first";
		return false;
	}
	const bool bgra = src.Format == DXGI_FORMAT_B8G8R8A8_UNORM || src.Format == DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
	const bool rgba = src.Format == DXGI_FORMAT_R8G8B8A8_UNORM || src.Format == DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	if (!bgra && !rgba) {
		Log::e() << "mzglD3D11ReadTexture: unsupported format " << (int) src.Format;
		return false;
	}

	D3D11_TEXTURE2D_DESC d	 = src;
	d.Width					 = (UINT) w;
	d.Height				 = (UINT) h;
	d.MipLevels				 = 1;
	d.ArraySize				 = 1;
	d.Usage					 = D3D11_USAGE_STAGING;
	d.BindFlags				 = 0;
	d.CPUAccessFlags		 = D3D11_CPU_ACCESS_READ;
	d.MiscFlags				 = 0;
	ID3D11Texture2D *staging = nullptr;
	HRESULT hr				 = device->CreateTexture2D(&d, nullptr, &staging);
	if (FAILED(hr) || staging == nullptr) {
		logHr("CreateTexture2D(staging)", hr);
		return false;
	}

	D3D11_BOX box = {(UINT) x, (UINT) y, 0, (UINT) (x + w), (UINT) (y + h), 1};
	ctx->CopySubresourceRegion(staging, 0, 0, 0, 0, tex, 0, &box);

	D3D11_MAPPED_SUBRESOURCE mapped = {};
	hr								= ctx->Map(staging, 0, D3D11_MAP_READ, 0, &mapped);
	if (FAILED(hr)) {
		logHr("Map(staging)", hr);
		staging->Release();
		return false;
	}
	outRgba.resize((size_t) w * h * 4);
	for (int row = 0; row < h; row++) {
		const uint8_t *s = static_cast<const uint8_t *>(mapped.pData) + (size_t) row * mapped.RowPitch;
		uint8_t *o		 = outRgba.data() + (size_t) row * w * 4;
		for (int px = 0; px < w; px++) {
			if (bgra) {
				o[px * 4 + 0] = s[px * 4 + 2];
				o[px * 4 + 1] = s[px * 4 + 1];
				o[px * 4 + 2] = s[px * 4 + 0];
			} else {
				o[px * 4 + 0] = s[px * 4 + 0];
				o[px * 4 + 1] = s[px * 4 + 1];
				o[px * 4 + 2] = s[px * 4 + 2];
			}
			o[px * 4 + 3] = 255;
		}
	}
	ctx->Unmap(staging, 0);
	staging->Release();
	return true;
}

D3D11Context::~D3D11Context() {
	shutdown();
}

bool D3D11Context::init(HWND hwnd, int width, int height, int sampleCount, ID3D11Device *sharedDevice) {
	this->hwnd		  = hwnd;
	this->width		  = width;
	this->height	  = height;
	this->sampleCount = sampleCount;

	const DXGI_SWAP_CHAIN_DESC swapChainDesc = swapChainDescFor(width, height);

	if (sharedDevice != nullptr) {
		device = sharedDevice;
		device->AddRef();
		device->GetImmediateContext(&deviceContext);
	} else {
		D3D_FEATURE_LEVEL featureLevel;
		D3D_FEATURE_LEVEL featureLevels[] = {D3D_FEATURE_LEVEL_11_0};

		UINT createDeviceFlags = 0;
		debugLayer			   = false;
		// DEBUG builds get the validation layer; MZGL_D3D11_DEBUG=1 in the
		// environment turns it on in any build (the messages go to the log)
#	ifdef DEBUG
		createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#	else
		if (const char *env = getenv("MZGL_D3D11_DEBUG"); env != nullptr && env[0] == '1') {
			createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
		}
#	endif
		HRESULT hr = D3D11CreateDevice(nullptr,
									   D3D_DRIVER_TYPE_HARDWARE,
									   nullptr,
									   createDeviceFlags,
									   featureLevels,
									   1,
									   D3D11_SDK_VERSION,
									   &device,
									   &featureLevel,
									   &deviceContext);
		if (SUCCEEDED(hr) && (createDeviceFlags & D3D11_CREATE_DEVICE_DEBUG)) {
			debugLayer = true;
		} else if (FAILED(hr) && (createDeviceFlags & D3D11_CREATE_DEVICE_DEBUG)) {
			// the SDK layers aren't installed on this machine - fail soft
			Log::d() << "D3D11 debug layer unavailable (0x" << std::hex << (unsigned long) hr << std::dec
					 << "), creating a plain device";
			createDeviceFlags &= ~D3D11_CREATE_DEVICE_DEBUG;
			hr = D3D11CreateDevice(nullptr,
								   D3D_DRIVER_TYPE_HARDWARE,
								   nullptr,
								   createDeviceFlags,
								   featureLevels,
								   1,
								   D3D11_SDK_VERSION,
								   &device,
								   &featureLevel,
								   &deviceContext);
		}
		if (FAILED(hr)) {
			logHr("D3D11CreateDevice", hr);
			return false;
		}
		Log::d() << "D3D11 device created, feature level: " << std::hex << featureLevel << std::dec
				 << (debugLayer ? " (debug layer on)" : "");
	}

	if (!createSwapChain(swapChainDesc)) {
		// No swap chain (typically no display attached to the adapter: a
		// headless CI box, a disconnected remote session): render into an
		// offscreen texture instead so the app keeps running, screenshots keep
		// working and present() keeps retrying the swap chain.
		Log::e() << "D3D11Context: no swap chain available, rendering offscreen";
		offscreen = true;
	}
	createRenderTarget();
	return true;
}

bool D3D11Context::recreate(ID3D11Device *sharedDevice) {
	const HWND h  = hwnd;
	const int w	  = width;
	const int hgt = height;
	const int sc  = sampleCount;
	shutdown();
	return init(h, w, hgt, sc, sharedDevice);
}

// The swap chain has to come from the DXGI factory that made the device's
// adapter: device -> IDXGIDevice -> adapter -> factory.
bool D3D11Context::createSwapChain(const DXGI_SWAP_CHAIN_DESC &desc, bool logFailure) {
	IDXGIDevice *dxgiDevice = nullptr;
	IDXGIAdapter *adapter	= nullptr;
	IDXGIFactory *factory	= nullptr;
	HRESULT hr				= device->QueryInterface(__uuidof(IDXGIDevice), (void **) &dxgiDevice);
	if (SUCCEEDED(hr)) hr = dxgiDevice->GetAdapter(&adapter);
	if (SUCCEEDED(hr)) hr = adapter->GetParent(__uuidof(IDXGIFactory), (void **) &factory);
	if (SUCCEEDED(hr)) {
		DXGI_SWAP_CHAIN_DESC mutableDesc = desc;
		hr								 = factory->CreateSwapChain(device, &mutableDesc, &swapChain);
	}
	if (factory) factory->Release();
	if (adapter) adapter->Release();
	if (dxgiDevice) dxgiDevice->Release();
	if (FAILED(hr)) {
		if (logFailure) logHr("CreateSwapChain", hr);
		return false;
	}
	return true;
}

DXGI_SWAP_CHAIN_DESC D3D11Context::swapChainDescFor(int w, int h) const {
	DXGI_SWAP_CHAIN_DESC d;
	memset(&d, 0, sizeof(d));
	d.BufferCount						 = 2;
	d.BufferDesc.Width					 = w;
	d.BufferDesc.Height					 = h;
	d.BufferDesc.Format					 = DXGI_FORMAT_B8G8R8A8_UNORM;
	d.BufferDesc.RefreshRate.Numerator	 = 60;
	d.BufferDesc.RefreshRate.Denominator = 1;
	d.BufferUsage						 = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	d.OutputWindow						 = hwnd;
	d.SampleDesc.Count					 = 1; // swap chain is always non-MSAA
	d.SampleDesc.Quality				 = 0;
	d.Windowed							 = TRUE;
	d.SwapEffect						 = DXGI_SWAP_EFFECT_DISCARD;
	d.Flags								 = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	return d;
}

// offscreen only: see whether a display has turned up since
void D3D11Context::tryRestoreSwapChain() {
	if (!offscreen || device == nullptr) return;
	const DXGI_SWAP_CHAIN_DESC desc = swapChainDescFor(width, height);
	if (!createSwapChain(desc, false)) return;
	Log::d() << "D3D11Context: swap chain available again, leaving offscreen mode";
	offscreen = false;
	releaseRenderTarget();
	createRenderTarget();
}

void D3D11Context::createRenderTarget() {
	HRESULT hr = S_OK;
	if (swapChain != nullptr) {
		// Get back buffer and create non-MSAA render target view
		hr = swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void **) &backBufferTexture);
		if (FAILED(hr) || backBufferTexture == nullptr) {
			logHr("GetBuffer(back buffer)", hr);
			return;
		}
	} else {
		// offscreen stand-in for the back buffer (same format as a swap chain's)
		D3D11_TEXTURE2D_DESC bb;
		memset(&bb, 0, sizeof(bb));
		bb.Width			= width;
		bb.Height			= height;
		bb.MipLevels		= 1;
		bb.ArraySize		= 1;
		bb.Format			= DXGI_FORMAT_B8G8R8A8_UNORM;
		bb.SampleDesc.Count = 1;
		bb.Usage			= D3D11_USAGE_DEFAULT;
		bb.BindFlags		= D3D11_BIND_RENDER_TARGET;
		hr					= device->CreateTexture2D(&bb, nullptr, &backBufferTexture);
		if (FAILED(hr) || backBufferTexture == nullptr) {
			logHr("CreateTexture2D(offscreen back buffer)", hr);
			return;
		}
	}
	hr = device->CreateRenderTargetView(backBufferTexture, nullptr, &renderTargetView);
	if (FAILED(hr)) logHr("CreateRenderTargetView(back buffer)", hr);

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

		hr = device->CreateTexture2D(&msaaDesc, nullptr, &msaaColorTexture);
		if (FAILED(hr)) logHr("CreateTexture2D(MSAA)", hr);
		hr = device->CreateRenderTargetView(msaaColorTexture, nullptr, &msaaRenderTargetView);
		if (FAILED(hr)) logHr("CreateRenderTargetView(MSAA)", hr);
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
	if (backBufferTexture) {
		backBufferTexture->Release();
		backBufferTexture = nullptr;
	}
}

void D3D11Context::resize(int newWidth, int newHeight) {
	if (newWidth == 0 || newHeight == 0) return;
	if (device == nullptr) return;
	width  = newWidth;
	height = newHeight;

	releaseRenderTarget();

	if (swapChain != nullptr) {
		const HRESULT hr = swapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);
		if (FAILED(hr)) logHr("ResizeBuffers", hr);
	}

	createRenderTarget();
}

HRESULT D3D11Context::present(bool vsync) {
	if (swapChain == nullptr) {
		if (!offscreen) return E_FAIL;
		// nothing to show; see whether a display has appeared every couple of seconds
		const ULONGLONG now = GetTickCount64();
		if (now - lastSwapChainRetryMs > 2000) {
			lastSwapChainRetryMs = now;
			tryRestoreSwapChain();
		}
		return S_OK;
	}
	return swapChain->Present(vsync ? 1 : 0, 0);
}

bool D3D11Context::readBackBuffer(std::vector<uint8_t> &outRgba) {
	return mzglD3D11ReadTexture(device, deviceContext, backBufferTexture, 0, 0, width, height, outRgba);
}

void D3D11Context::shutdown() {
	releaseRenderTarget();
	offscreen = false;
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
