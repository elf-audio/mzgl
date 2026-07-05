#pragma once

// Texture for the native Metal backend. Textures live in a process-global
// registry keyed by a uint32_t id (mirroring sokol's image handles) so the
// rest of mzgl can keep passing texture ids around (Font atlas etc). The
// MTLTexture itself is only touched from MetalTexture.mm.

#include "Texture.h"
#include <cstdint>

class MetalTexture : public Texture {
public:
	MetalTexture(Graphics &g);

	// wrap an existing registry texture (e.g. the font atlas) - does not own it
	MetalTexture(Graphics &g, uint32_t textureID, int width, int height);

	~MetalTexture() override;

	void setSamplingMethod(Texture::SamplingMethod sampling) override {
		linearFilter = sampling == Texture::SamplingMethod::Linear;
	}
	void enableWrap(bool wrapX, bool wrapY) override {
		this->wrapX = wrapX;
		this->wrapY = wrapY;
	}

	void allocate(const unsigned char *data, int w, int h, Texture::PixelFormat fmt) override;

	// re-upload the full texture in the format it was allocated with (used by
	// the fontstash for atlas updates)
	void updateData(const unsigned char *data);

	void createMipMaps() override {}
	void bind() override;
	void unbind() override;
	void deallocate() override;
	[[nodiscard]] bool allocated() const override { return false; }

	[[nodiscard]] uint32_t getId() const override { return textureId; }

	// packs the sampler state into a small key for the sampler cache
	[[nodiscard]] uint32_t samplerKey() const {
		return (linearFilter ? 1u : 0u) | (wrapX ? 2u : 0u) | (wrapY ? 4u : 0u);
	}

private:
	uint32_t textureId = 0;
	bool linearFilter  = true;
	bool wrapX		   = false;
	bool wrapY		   = false;
	bool isLuminance   = false;
};
