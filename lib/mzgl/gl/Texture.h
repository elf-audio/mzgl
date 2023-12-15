//
//  Texture.hpp
//  MZGL
//
//  Created by Marek Bereza on 15/01/2018.
//  Copyright © 2018 Marek Bereza. All rights reserved.
//

#pragma once
#include <string>
#include <memory>
#include <vector>
#include "Image.h"

#include "Rectf.h"

class Graphics;
class Texture;

typedef std::shared_ptr<Texture> TextureRef;

class Texture {
public:
	enum class SamplingMethod { Nearest, Linear };

	enum class PixelFormat {
		RGBA,
		RGB,
		LUMINANCE,
	};

	void setSamplingMethod(SamplingMethod sampling);

	static TextureRef create(uint32_t textureID = 0, int width = 0, int height = 0) {
		return TextureRef(new Texture(textureID, width, height));
	}

	static TextureRef create(std::string path) {
		auto ref = TextureRef(new Texture());
		if (ref->load(path)) {
			return ref;
		}
		return nullptr;
	}

	static TextureRef create(const std::vector<unsigned char> &pngData) {
		return TextureRef(new Texture(pngData));
	}
	// WARNING - img gets modified inplace if it's an incompatible number of channels
	static TextureRef create(ImageRef img) {
		auto tex = TextureRef(new Texture());
		tex->loadFromPixels(
			img->data, img->width, img->height, img->numChannels, img->bytesPerChannel, img->isFloat);
		return tex;
	}

	void enableWrap(bool wrapX = true, bool wrapY = true);

	void allocate(const unsigned char *data, int w, int h, PixelFormat type = PixelFormat::RGBA);
	void allocate(int w, int h, PixelFormat type = PixelFormat::RGBA);
	void deallocate();
	bool load(const std::string &imgFilePath);

	int width  = 0;
	int height = 0;
	uint32_t getId() { return textureID; }
	// TODO: this owns thing is a bit messy, surely we want shared_ptr
	// here. It's a case when you put a texID in the constructor from
	// somewhere else to draw it temporarily, we don't want the tex
	// to delete when out of scope.
	bool owns = false;
	void bind();
	void unbind();

	void draw(Graphics &g, const Rectf &r) { draw(g, r.x, r.y, r.width, r.height); }

	void draw(Graphics &g, float x = 0.f, float y = 0.f) { draw(g, x, y, (float) width, (float) height); }

	void draw(Graphics &g, float x, float y, float width, float height);

	virtual ~Texture();

	bool allocated() { return textureID != 0; }
	void createMipMaps();

#ifdef __ANDROID__
	static std::vector<Texture *> textures;
#endif
private:
	// WARNING - outData gets modified inplace if it's an incompatible number of channels
	bool loadFromPixels(std::vector<uint8_t> &outData, int w, int h, int numChans, int bytesPerChan, bool isFloat);

	Texture(uint32_t textureID, int width, int height)
		: Texture() {
		this->textureID = textureID;
		this->width		= width;
		this->height	= height;
		owns			= false;
	}

	Texture(const std::vector<unsigned char> &pngData);

	Texture();
	uint32_t textureID = 0;
};
