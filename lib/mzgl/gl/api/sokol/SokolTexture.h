#pragma once

#include "SokolAPI.h"
#include "Texture.h"
class SokolTexture : public Texture {
public:
	SokolAPI &api;
	SokolTexture(Graphics &g)
		: Texture(g)
		, api(dynamic_cast<SokolAPI &>(g.getAPI())) {}

	SokolTexture(Graphics &g, uint32_t textureID, int width, int height)
		: SokolTexture(g) {
		// TODO: untested
		owns		 = false;
		this->width	 = width;
		this->height = height;
		image		 = {textureID};
	}

	void setSamplingMethod(Texture::SamplingMethod sampling) override {
		auto newFilter = sampling == Texture::SamplingMethod::Linear ? SG_FILTER_LINEAR : SG_FILTER_NEAREST;
		if (newFilter == minFilter && newFilter == magFilter) {
			return;
		}
		minFilter = newFilter;
		magFilter = minFilter;
		sampler	  = {0};
	};
	void enableWrap(bool wrapX, bool wrapY) override {
		auto newXWrap = wrapX ? SG_WRAP_REPEAT : SG_WRAP_CLAMP_TO_EDGE;
		auto newYWrap = wrapY ? SG_WRAP_REPEAT : SG_WRAP_CLAMP_TO_EDGE;
		if (newXWrap == xWrap && newYWrap == yWrap) {
			return;
		}

		xWrap	= newXWrap;
		yWrap	= newYWrap;
		sampler = {0};
	}

	sg_wrap xWrap {SG_WRAP_CLAMP_TO_EDGE};
	sg_wrap yWrap {SG_WRAP_CLAMP_TO_EDGE};

	sg_filter minFilter {SG_FILTER_LINEAR};
	sg_filter magFilter {SG_FILTER_LINEAR};

	void allocate(const unsigned char *data, int w, int h, Texture::PixelFormat fmt) override {
		// Sokol/D3D11 only supports RGBA8 textures - convert RGB to RGBA if needed
		const unsigned char *pixelData = data;
		std::vector<unsigned char> rgbaData;
		if (fmt == Texture::PixelFormat::RGB) {
			rgbaData.resize(w * h * 4);
			for (int i = 0; i < w * h; i++) {
				rgbaData[i * 4 + 0] = data[i * 3 + 0];
				rgbaData[i * 4 + 1] = data[i * 3 + 1];
				rgbaData[i * 4 + 2] = data[i * 3 + 2];
				rgbaData[i * 4 + 3] = 255;
			}
			pixelData = rgbaData.data();
		}
		sg_range dataRange		= {};
		dataRange.ptr			= pixelData;
		dataRange.size			= static_cast<size_t>(w * h * 4);
		sg_image_desc imageDesc = {};
		imageDesc.width			= w;
		imageDesc.height		= h;
		imageDesc.pixel_format	= SG_PIXELFORMAT_RGBA8;
		imageDesc.data.subimage[0][0] = dataRange;
		image = sg_make_image(imageDesc);
	}
	void createMipMaps() override {}
	void bind() override {
		bound = true;
		if (sampler.id == 0) {
			sg_sampler_desc samplerDesc = {};
			samplerDesc.min_filter		= minFilter;
			samplerDesc.mag_filter		= magFilter;
			samplerDesc.wrap_u			= xWrap;
			samplerDesc.wrap_v			= yWrap;
			sampler						= api.createOrReuseSampler(samplerDesc);
		}

		api.setBoundTexture(image, sampler);
	}
	void unbind() override {
		bound = false;
		api.setBoundTexture({0}, {0});
	}
	void deallocate() override { deallocateImage(); }
	bool allocated() const override { return false; }

	uint32_t getId() const override { return image.id; }

	~SokolTexture() {
		if (bound) unbind();
		deallocate();
	}

private:
	void deallocateImage() {
		if (sampler.id != 0) {
			sg_destroy_sampler(sampler);
			sampler.id = 0;
		}
	}
	bool bound = false;
	sg_image image	   = {};
	sg_sampler sampler = {};
};