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

	virtual void setSamplingMethod(SamplingMethod sampling)												 = 0;
	virtual void bind()																					 = 0;
	virtual void unbind()																				 = 0;
	virtual void enableWrap(bool wrapX = true, bool wrapY = true)										 = 0;
	virtual uint32_t getId() const																		 = 0;
	virtual bool allocated() const																		 = 0;
	virtual void deallocate()																			 = 0;
	virtual void allocate(const unsigned char *data, int w, int h, PixelFormat type = PixelFormat::RGBA) = 0;
	virtual void createMipMaps()																		 = 0;

	static TextureRef create(Graphics &g, uint32_t textureID = 0, int width = 0, int height = 0);
	
	static TextureRef create(Graphics &g, std::string path);

	static TextureRef create(Graphics &g, const std::vector<unsigned char> &pngData);
	// WARNING - img gets modified inplace if it's an incompatible number of channels
	static TextureRef create(Graphics &g, ImageRef img);

	void allocate(int w, int h, PixelFormat type = PixelFormat::RGBA);

	bool load(const std::string &imgFilePath);

	int width  = 0;
	int height = 0;
	// TODO: this owns thing is a bit messy, surely we want shared_ptr
	// here. It's a case when you put a texID in the constructor from
	// somewhere else to draw it temporarily, we don't want the tex
	// to delete when out of scope.
	bool owns = false;

	void draw(const Rectf &r) { draw(r.x, r.y, r.width, r.height); }

	void draw(float x = 0.f, float y = 0.f) { draw(x, y, (float) width, (float) height); }

	void draw(float x, float y, float width, float height);

	virtual ~Texture();

#ifdef __ANDROID__
	static std::vector<Texture *> textures;
#endif
protected:
	Texture(Graphics &g, uint32_t textureID, int width, int height)
		: Texture(g) {}

	Texture(Graphics &g);
	Graphics &g;

private:
	// WARNING - outData gets modified inplace if it's an incompatible number of channels
	bool loadFromPixels(std::vector<uint8_t> &outData, int w, int h, int numChans, int bytesPerChan, bool isFloat);
};

class TextureBinding {
public:
	TextureBinding(TextureRef tex) {
		this->tex = tex;
		tex->bind();
	}

	~TextureBinding() { tex->unbind(); }

private:
	TextureRef tex;
};