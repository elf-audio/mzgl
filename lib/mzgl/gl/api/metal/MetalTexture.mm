#include "MetalTexture.h"
#include "MetalAPI.h"
#include "MetalContext.h"
#include "Graphics.h"
#include "log.h"
#include <vector>
#include <TargetConditionals.h>

MetalTexture::MetalTexture(Graphics &g)
	: Texture(g) {}

MetalTexture::MetalTexture(Graphics &g, uint32_t textureID, int width, int height)
	: Texture(g) {
	owns		 = false;
	this->width	 = width;
	this->height = height;
	textureId	 = textureID;
}

MetalTexture::~MetalTexture() {
	// only clear the API's binding if it still points at THIS texture -
	// another texture may have been bound over us since our bind()
	auto *api = static_cast<MetalAPI *>(&g.getAPI());
	if (api->getBoundTexture() == textureId && textureId != 0) {
		api->setBoundTexture(0, 0);
	}
	deallocate();
}

void MetalTexture::allocate(const unsigned char *data, int w, int h, Texture::PixelFormat fmt) {
	deallocate();
	this->width	 = w;
	this->height = h;
	owns		 = true;
	isLuminance	 = fmt == Texture::PixelFormat::LUMINANCE;

	MTLTextureDescriptor *desc =
		[MTLTextureDescriptor texture2DDescriptorWithPixelFormat:isLuminance ? MTLPixelFormatR8Unorm
																			 : MTLPixelFormatRGBA8Unorm
														   width:(NSUInteger) w
														  height:(NSUInteger) h
													   mipmapped:NO];
	desc.usage = MTLTextureUsageShaderRead;
#if TARGET_OS_OSX
	desc.storageMode = MTLStorageModeManaged;
#else
	desc.storageMode = MTLStorageModeShared;
#endif

	id<MTLTexture> tex = [mzglMetal::device() newTextureWithDescriptor:desc];
	if (tex == nil) {
		Log::e() << "MetalTexture::allocate failed (" << w << "x" << h << ")";
		return;
	}

	if (data != nullptr) {
		// RGB has no Metal format - expand to RGBA (same as the Sokol backend)
		const unsigned char *pixelData = data;
		std::vector<unsigned char> rgbaData;
		if (fmt == Texture::PixelFormat::RGB) {
			rgbaData.resize((size_t) w * h * 4);
			for (int i = 0; i < w * h; i++) {
				rgbaData[i * 4 + 0] = data[i * 3 + 0];
				rgbaData[i * 4 + 1] = data[i * 3 + 1];
				rgbaData[i * 4 + 2] = data[i * 3 + 2];
				rgbaData[i * 4 + 3] = 255;
			}
			pixelData = rgbaData.data();
		}
		const NSUInteger bytesPerRow = (NSUInteger) w * (isLuminance ? 1 : 4);
		[tex replaceRegion:MTLRegionMake2D(0, 0, (NSUInteger) w, (NSUInteger) h)
			   mipmapLevel:0
				 withBytes:pixelData
			   bytesPerRow:bytesPerRow];
	}

	textureId = mzglMetal::registerTexture(tex);
}

void MetalTexture::updateData(const unsigned char *data) {
	id<MTLTexture> tex = mzglMetal::lookupTexture(textureId);
	if (tex == nil || data == nullptr) return;
	const NSUInteger bytesPerRow = (NSUInteger) width * (isLuminance ? 1 : 4);
	[tex replaceRegion:MTLRegionMake2D(0, 0, (NSUInteger) width, (NSUInteger) height)
		   mipmapLevel:0
			 withBytes:data
		   bytesPerRow:bytesPerRow];
}

void MetalTexture::bind() {
	auto *api = static_cast<MetalAPI *>(&g.getAPI());
	api->setBoundTexture(textureId, samplerKey());
}

void MetalTexture::unbind() {
	auto *api = static_cast<MetalAPI *>(&g.getAPI());
	api->setBoundTexture(0, 0);
}

void MetalTexture::deallocate() {
	if (owns && textureId != 0) {
		mzglMetal::unregisterTexture(textureId);
	}
	textureId = 0;
}
