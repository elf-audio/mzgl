//
//  Texture.cpp
//  MZGL
//
//  Created by Marek Bereza on 15/01/2018.
//  Copyright © 2018 Marek Bereza. All rights reserved.
//

#include "Texture.h"
#include "Vbo.h"
#include <vector>
#include "Graphics.h"
#include "error.h"
#include "log.h"
#include "mzAssert.h"

#ifdef __ANDROID__
std::vector<Texture *> Texture::textures;
#endif

#include "Image.h"

#ifdef MZGL_SOKOL
#	include "SokolTexture.h"
#	define TEXTURE_CLASS SokolTexture
#else
#	include "OpenGLTexture.h"
#	define TEXTURE_CLASS OpenGLTexture
#endif

Texture::Texture(Graphics &g)
	: g(g) {
#ifdef __ANDROID__
	textures.push_back(this);
#endif
}
TextureRef Texture::create(Graphics &g, uint32_t textureID, int width, int height) {
	return TextureRef(new TEXTURE_CLASS(g, textureID, width, height));
}

TextureRef Texture::create(Graphics &g, std::string path) {
	auto ref = TextureRef(new TEXTURE_CLASS(g));
	if (ref->load(path)) {
		return ref;
	}
	return nullptr;
}

TextureRef Texture::create(Graphics &g, const std::vector<unsigned char> &pngData) {
	std::vector<uint8_t> outData;
	int w;
	int h;
	int numChans;
	int bytesPerChan;

	if (!Image::loadPngFromData(pngData, outData, w, h, numChans, bytesPerChan)) {
		Log::e() << "Couldn't load image from png data!!";
		return nullptr;
	}

	bool isFloat = false;

	auto tex = Texture::create(g);
	tex->loadFromPixels(outData, w, h, numChans, bytesPerChan, isFloat);
	return tex;
}

Texture::~Texture() {
#ifdef __ANDROID__
	for (int i = 0; i < textures.size(); i++) {
		if (textures[i] == this) {
			textures.erase(textures.begin() + i);
			break;
		}
	}
#endif
}

bool Texture::load(const std::string &imgFilePath) {
	owns = true;

	std::vector<uint8_t> outData;
	int w			 = 0;
	int h			 = 0;
	int numChans	 = 0;
	int bytesPerChan = 0;
	bool isFloat	 = false;

	if (!Image::load(imgFilePath, outData, w, h, numChans, bytesPerChan, isFloat)) {
		Log::e() << "Couldn't load image at " << imgFilePath;
		return false;
	}

	return loadFromPixels(outData, w, h, numChans, bytesPerChan, isFloat);
}

static Texture::PixelFormat numChansToPixelFormat(int numChans) {
	if (numChans == 4) {
		return Texture::PixelFormat::RGBA;
	} else if (numChans == 3) {
		return Texture::PixelFormat::RGB;
	} else if (numChans == 1) {
		return Texture::PixelFormat::LUMINANCE;
	}
	Log::e() << "ERROR: got number of channels don't know how to deal with: " << numChans;
	return Texture::PixelFormat::RGBA;
}

bool Texture::loadFromPixels(
	std::vector<uint8_t> &outData, int w, int h, int numChans, int bytesPerChan, bool isFloat) {
	if (w <= 0 || h <= 0) {
		Log::e() << "Texture::loadFromPixels() : image dims 0";
		return false;
	}
	if (numChans < 1 || numChans > 4) {
		Log::e() << "Texture::loadFromPixels() : unsupported channel count: " << numChans;
		return false;
	}
	if (bytesPerChan == 2) {
		Log::d() << "Texture only supports 8-bit textures, this is 16 - we converting to 8";
		size_t npix	 = static_cast<size_t>(w) * static_cast<size_t>(h) * static_cast<size_t>(numChans);
		uint16_t *sh = (uint16_t *) outData.data();
		for (size_t i = 0; i < npix; i++) {
			outData[i] = sh[i] / 256;
		}
		outData.resize(outData.size() / 2);
	}
	mzAssert(!isFloat);

	// Verify the pixel buffer is big enough for the declared dimensions before
	// we hand the pointer to glTexImage2D; the GPU driver reads w*h*numChans
	// bytes off the pointer with no further bounds checks, so an undersized
	// buffer (corrupt PNG, channel count mismatch, etc.) faults inside the
	// driver — matching the win-crash 5ae344c1 GPU-driver cluster.
	const size_t expected = static_cast<size_t>(w) * static_cast<size_t>(h) * static_cast<size_t>(numChans);
	if (outData.size() < expected) {
		Log::e() << "Texture::loadFromPixels() : buffer too small for "
				 << w << "x" << h << "x" << numChans << " (have " << outData.size()
				 << " bytes, need " << expected << ")";
		return false;
	}

	if (numChans == 1) {
		// Y -> RGB
		outData.resize(3 * outData.size());
		for (int i = w * h - 1; i >= 0; --i) {
			outData[i * 3 + 2] = outData[i * 3 + 1] = outData[i * 3] = outData[i];
		}
		numChans = 3;
	} else if (numChans == 2) {
		// YA -> RGBA. Without this branch the old code fell through to an
		// mzAssert (no-op in release) and uploaded w*h*2 bytes while telling
		// the driver the texture was RGBA (w*h*4) — read-past-buffer in the
		// driver.
		std::vector<uint8_t> rgba(static_cast<size_t>(w) * static_cast<size_t>(h) * 4);
		for (int i = 0; i < w * h; ++i) {
			uint8_t y			= outData[i * 2];
			uint8_t a			= outData[i * 2 + 1];
			rgba[i * 4 + 0]		= y;
			rgba[i * 4 + 1]		= y;
			rgba[i * 4 + 2]		= y;
			rgba[i * 4 + 3]		= a;
		}
		outData	 = std::move(rgba);
		numChans = 4;
	}

	mzAssert(numChans == 3 || numChans == 4);

	allocate(outData.data(), w, h, numChansToPixelFormat(numChans));
	return true;
}

void Texture::allocate(int w, int h, PixelFormat type) {
	allocate(nullptr, w, h);
}

void Texture::draw(float x, float y, float width, float height) {
	VboRef vbo = Vbo::create();

	Rectf r(x, y, width, height);
	Rectf t(0, 0, 1, 1);
	std::vector<glm::vec2> verts	 = {r.tl(), r.tr(), r.br(), r.br(), r.bl(), r.tl()};
	std::vector<glm::vec2> texCoords = {t.tl(), t.tr(), t.br(), t.br(), t.bl(), t.tl()};

	vbo->setVertices(verts);
	vbo->setTexCoords(texCoords);

	// Scoped bind so the global texture-binding state is left clean. The Sokol backend
	// tracks the bound texture globally and asserts if a later VBO without tex coords
	// (e.g. drawRoundedRect) is drawn while a texture is still bound; without unbinding,
	// any tex->draw() leaks its binding into the next non-textured draw and aborts.
	// (The font renderer binds the same way; on GL the unbind is a harmless reset.)
	TextureBinding binding(*this);
	g.texShader->begin();
	vbo->draw(g);
}
