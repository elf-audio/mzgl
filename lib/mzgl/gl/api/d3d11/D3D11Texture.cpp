#include "D3D11Texture.h"
#include "D3D11API.h"
#include "D3D11Internal.h"
#include "Graphics.h"
#include "log.h"
#include <vector>

D3D11Texture::D3D11Texture(Graphics &g)
	: Texture(g) {
}

D3D11Texture::D3D11Texture(Graphics &g, uint32_t textureID, int width, int height)
	: Texture(g) {
	owns		 = false;
	this->width	 = width;
	this->height = height;
	textureId	 = textureID;
}

D3D11Texture::~D3D11Texture() {
	// only clear the API's binding if it still points at THIS texture -
	// another texture may have been bound over us since our bind()
	auto *api = static_cast<D3D11API *>(&g.getAPI());
	if (api->getBoundTexture() == textureId && textureId != 0) {
		api->setBoundTexture(0, 0);
	}
	deallocate();
}

void D3D11Texture::allocate(const unsigned char *data, int w, int h, Texture::PixelFormat fmt) {
	deallocate();
	this->width	 = w;
	this->height = h;
	owns		 = true;
	isLuminance	 = fmt == Texture::PixelFormat::LUMINANCE;

	ID3D11Device *dev = mzglD3D11::device();
	if (dev == nullptr) {
		Log::e() << "D3D11Texture::allocate: no device yet (" << w << "x" << h << ")";
		return;
	}

	// RGB has no D3D11 format - expand to RGBA (same as the Sokol backend)
	const unsigned char *pixelData = data;
	std::vector<unsigned char> rgbaData;
	if (data != nullptr && fmt == Texture::PixelFormat::RGB) {
		rgbaData.resize((size_t) w * h * 4);
		for (int i = 0; i < w * h; i++) {
			rgbaData[i * 4 + 0] = data[i * 3 + 0];
			rgbaData[i * 4 + 1] = data[i * 3 + 1];
			rgbaData[i * 4 + 2] = data[i * 3 + 2];
			rgbaData[i * 4 + 3] = 255;
		}
		pixelData = rgbaData.data();
	}

	D3D11_TEXTURE2D_DESC d = {};
	d.Width				   = (UINT) w;
	d.Height			   = (UINT) h;
	d.MipLevels			   = 1;
	d.ArraySize			   = 1;
	d.Format			   = isLuminance ? DXGI_FORMAT_R8_UNORM : DXGI_FORMAT_R8G8B8A8_UNORM;
	d.SampleDesc.Count	   = 1;
	d.SampleDesc.Quality   = 0;
	d.Usage				   = D3D11_USAGE_DEFAULT;
	d.BindFlags			   = D3D11_BIND_SHADER_RESOURCE;

	D3D11_SUBRESOURCE_DATA sd = {};
	sd.pSysMem				  = pixelData;
	sd.SysMemPitch			  = (UINT) w * (isLuminance ? 1 : 4);

	mzglD3D11::TextureEntry entry;
	entry.width	 = w;
	entry.height = h;
	entry.format = d.Format;
	if (!mzglD3D11Check(dev->CreateTexture2D(&d, pixelData != nullptr ? &sd : nullptr, &entry.texture),
						"CreateTexture2D")) {
		Log::e() << "D3D11Texture::allocate failed (" << w << "x" << h << ")";
		return;
	}
	if (!mzglD3D11Check(dev->CreateShaderResourceView(entry.texture, nullptr, &entry.srv),
						"CreateShaderResourceView")) {
		mzglD3D11Release(entry.texture);
		return;
	}
	textureId = mzglD3D11::registerTexture(entry);
}

void D3D11Texture::updateData(const unsigned char *data) {
	mzglD3D11::TextureEntry entry;
	if (data == nullptr || !mzglD3D11::lookupTextureEntry(textureId, entry)) return;
	auto *dc = mzglD3D11::context();
	if (dc == nullptr) return;
	const UINT rowPitch = (UINT) width * (isLuminance ? 1 : 4);
	dc->UpdateSubresource(entry.texture, 0, nullptr, data, rowPitch, 0);
}

void D3D11Texture::bind() {
	auto *api = static_cast<D3D11API *>(&g.getAPI());
	api->setBoundTexture(textureId, samplerKey());
}

void D3D11Texture::unbind() {
	auto *api = static_cast<D3D11API *>(&g.getAPI());
	api->setBoundTexture(0, 0);
}

void D3D11Texture::deallocate() {
	if (owns && textureId != 0) {
		mzglD3D11::unregisterTexture(textureId);
	}
	textureId = 0;
}
