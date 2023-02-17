//
//  Fbo.hpp
//  MZGL
//
//  Created by Marek Bereza on 27/02/2018.
//  Copyright © 2018 Marek Bereza. All rights reserved.
//

#pragma once
#include "Texture.h"

class Graphics;
// with help from http://www.songho.ca/opengl/gl_fbo.html
class Fbo {
public:
	
	
	int width = 0;
	int height = 0;
	TextureRef tex;
	
	void setup(int w, int h, Texture::PixelFormat format = Texture::PixelFormat::RGBA, bool hasDepth = false, int numSamples = 0);
	
	void begin(Graphics &g);
	void end(Graphics &g);
	
	// these are for binding like a texture
	void bind();
	void unbind();
	
	
	void readPixels(Graphics &g, std::vector<uint8_t> &data);
	
	~Fbo();
private:
	int numSamples = 0;

	// GLuints
	uint32_t fboId = 0;
	uint32_t rboDepthId = 0;
	
	// when msaa engaged, this is where the multisampled texture is stored.
	uint32_t rboMsaaColorId = 0;
	// when msaa, this is the normal sized buffer, backed by our texture, that gets blitted into
	uint32_t downsampleFboId = 0;
	uint32_t downsampleDepthRboId = 0;
	void deallocate();
	
};
