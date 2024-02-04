//
//  Texture.cpp
//  MZGL
//
//  Created by Marek Bereza on 15/01/2018.
//  Copyright © 2018 Marek Bereza. All rights reserved.
//

#include "Texture.h"
//#include "picopng.hpp"
#include "Vbo.h"
#include <vector>
#include "Graphics.h"
#include "error.h"
#include "log.h"
#include "mzAssert.h"
#include "mzOpenGL.h"

using namespace std;

#ifdef __ANDROID__
vector<Texture *> Texture::textures;
#endif

Texture::Texture() {
#ifdef __ANDROID__
	textures.push_back(this);
#endif
}

#include "Image.h"

void Texture::setSamplingMethod(Texture::SamplingMethod sampling) {
	bind();
	if (sampling == Texture::SamplingMethod::Linear) {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	} else if (sampling == Texture::SamplingMethod::Nearest) {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	} else {
		Log::e() << "unknown sampling method";
	}
}

Texture::Texture(const std::vector<unsigned char> &pngData) {
#ifdef __ANDROID__
	textures.push_back(this);
#endif
	owns = true;

	vector<uint8_t> outData;
	int w;
	int h;
	int numChans;
	int bytesPerChan;
	bool isFloat;

	if (!Image::loadPngFromData(pngData, outData, w, h, numChans, bytesPerChan, isFloat)) {
		Log::e() << "Couldn't load image!!";
		return;
	}

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
	deallocate();
}

bool Texture::load(const string &imgFilePath) {
	owns = true;

	vector<uint8_t> outData;
	int w			 = 0;
	int h			 = 0;
	int numChans	 = 0;
	int bytesPerChan = 0;
	bool isFloat	 = false;

	if (!Image::load(imgFilePath, outData, w, h, numChans, bytesPerChan, isFloat)) {
		Log::e() << "Couldn't load image!!";
		return false;
	}

	return loadFromPixels(outData, w, h, numChans, bytesPerChan, isFloat);
}

//static PixelFormat glToPixelFormat(GLuint fmt) {
//	int glType = GL_RGBA;
//	if(numChans==4) {
//		glType = GL_RGBA;
//	} else if(numChans==3) {
//		glType = GL_RGB;
//	} else if(numChans==1) {
//#ifdef GL_LUMINANCE
//		glType = GL_LUMINANCE;
//#else
//		glType = GL_ALPHA;
//#endif
//
//	}
//}

static GLenum pixelFormatToGLType(Texture::PixelFormat fmt) {
	switch (fmt) {
		case Texture::PixelFormat::RGBA: return GL_RGBA;
		case Texture::PixelFormat::RGB: return GL_RGB;
		case Texture::PixelFormat::LUMINANCE:
#ifdef GL_LUMINANCE
			return GL_LUMINANCE;
#else
			return GL_ALPHA;
#endif
		default: return GL_RGBA;
	}
}

static Texture::PixelFormat numChansToPixelFormat(int numChans) {
	int glType = GL_RGBA;
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
	vector<uint8_t> &outData, int w, int h, int numChans, int bytesPerChan, bool isFloat) {
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

	//    int glType = GL_RGBA;
	//    if(numChans==4) {
	//        glType = GL_RGBA;
	//    } else if(numChans==3) {
	//        glType = GL_RGB;
	//    } else if(numChans==1) {
	//#ifdef GL_LUMINANCE
	//        glType = GL_LUMINANCE;
	//#else
	//        glType = GL_ALPHA;
	//#endif
	//
	//    }

	allocate(outData.data(), w, h, numChansToPixelFormat(numChans));
	return true;
}

void Texture::allocate(int w, int h, PixelFormat type) {
	allocate(nullptr, w, h);
}

void Texture::allocate(const unsigned char *data, int w, int h, PixelFormat fmt) {
	owns = true;

	auto type = pixelFormatToGLType(fmt);
	GetError();
	glGenTextures(1, &textureID);

	GetError();

	// "Bind" the newly created texture : all future texture functions will modify this texture
	glBindTexture(GL_TEXTURE_2D, textureID);
	GetError();
	// Give the image to OpenGL
	GLuint internalFormat = GL_RGBA8;
	if (type == GL_RGB) {
		internalFormat = GL_RGB8;
	} else if (type == GL_RGBA) {
		internalFormat = GL_RGBA8;
	} else {
		mzAssert(0);
	}
#ifdef __ANDROID__ // this might be necessary for all platforms
	// https://stackoverflow.com/questions/58925604/glteximage2d-crashing-program
	// -- it's to do with rows being aligned - so if each row isn't aligned to 4 it needs this.
	if (type != GL_RGBA) {
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	}
#endif

	glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, w, h, 0, type, GL_UNSIGNED_BYTE, data);
	GetError();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	GetError();
	this->width	 = (int) w;
	this->height = (int) h;
	glBindTexture(GL_TEXTURE_2D, 0);
}

void Texture::enableWrap(bool wrapX, bool wrapY) {
	glBindTexture(GL_TEXTURE_2D, textureID);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapX ? GL_REPEAT : GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapY ? GL_REPEAT : GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_2D, 0);
}

void Texture::createMipMaps() {
	GetError();

	// "Bind" the newly created texture : all future texture functions will modify this texture
	glBindTexture(GL_TEXTURE_2D, textureID);
	GetError();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	GetError();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	GetError();

	glGenerateMipmap(GL_TEXTURE_2D);
	GetError();
	glBindTexture(GL_TEXTURE_2D, 0);
	GetError();
}
void Texture::bind() {
	glBindTexture(GL_TEXTURE_2D, textureID);
}

void Texture::unbind() {
	glBindTexture(GL_TEXTURE_2D, 0);
}
void Texture::draw(Graphics &g, float x, float y, float width, float height) {
	VboRef vbo = Vbo::create();

	Rectf r(x, y, width, height);
	Rectf t(0, 0, 1, 1);
	vector<glm::vec2> verts		= {r.tl(), r.tr(), r.br(), r.br(), r.bl(), r.tl()};
	vector<glm::vec2> texCoords = {t.tl(), t.tr(), t.br(), t.br(), t.bl(), t.tl()};

	vbo->setVertices(verts);
	vbo->setTexCoords(texCoords);

	GetError();
	bind();
	GetError();
	g.state.texShader->begin();
	vbo->draw(g);
	GetError();
}
void Texture::deallocate() {
	if (owns && textureID != 0) {
		glDeleteTextures(1, &textureID);
		textureID = 0;
	}
}
