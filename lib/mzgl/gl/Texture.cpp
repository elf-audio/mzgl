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
vector<Texture *> Texture::textures;
#endif

#include "Image.h"

#ifdef MZGL_SOKOL_METAL
#	include "SokolTexture.h"
//impl = std::make_unique<SokolTextureImpl>(g.getAPI());
#	define TEXTURE_CLASS SokolTexture
#else
#	include "OpenGLTexture.h"
#	define TEXTURE_CLASS OpenGLTexture
#endif

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
	loadFromPixels(outData, w, h, numChans, bytesPerChan, isFloat);
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
	if (w == 0 || h == 0) {
		Log::e() << "Texture::loadFromPixels() : image dims 0";
		return false;
	}
	if (bytesPerChan == 2) {
		Log::d() << "Texture only supports 8-bit textures, this is 16 - we converting to 8";
		int npix	 = w * h * numChans;
		uint16_t *sh = (uint16_t *) outData.data();
		for (int i = 0; i < npix; i++) {
			outData[i] = sh[i] / 256;
		}
		outData.resize(outData.size() / 2);
	}
	mzAssert(!isFloat);

	if (numChans == 1) {
		outData.resize(3 * outData.size());
		for (int i = w * h - 1; i >= 0; --i) {
			outData[i * 3 + 2] = outData[i * 3 + 1] = outData[i * 3] = outData[i];
		}
		numChans = 3;
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

	GetError();
	bind();
	GetError();
	g.texShader->begin();
	vbo->draw(g);
	GetError();
}
