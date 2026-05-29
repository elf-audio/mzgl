#pragma once

#include "mzOpenGL.h"
#include "Texture.h"

class OpenGLTexture : public Texture {
public:
	uint32_t textureID = 0;

	OpenGLTexture(Graphics &g)
		: Texture(g) {}
	OpenGLTexture(Graphics &g, uint32_t textureID, int width, int height)
		: Texture(g) {
		this->textureID = textureID;
		this->width		= width;
		this->height	= height;
		owns			= false;
	}

	void unbind() override { glBindTexture(GL_TEXTURE_2D, 0); }

	uint32_t getId() const override { return textureID; }
	void deallocate() override {
		if (!owns) return;
		if (textureID != 0) {
			glDeleteTextures(1, &textureID);
			textureID = 0;
		}
	}
	void setId(uint32_t id) { textureID = id; }

	bool allocated() const override { return textureID != 0; }
	~OpenGLTexture() override { deallocate(); }
	void createMipMaps() override {
		glBindTexture(GL_TEXTURE_2D, textureID);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glGenerateMipmap(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, 0);
	}
	void setSamplingMethod(Texture::SamplingMethod sampling) override {
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

	void enableWrap(bool wrapX, bool wrapY) override {
		glBindTexture(GL_TEXTURE_2D, textureID);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapX ? GL_REPEAT : GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapY ? GL_REPEAT : GL_CLAMP_TO_EDGE);
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	void bind() override { glBindTexture(GL_TEXTURE_2D, textureID); }
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

	void allocate(const unsigned char *data, int w, int h, Texture::PixelFormat fmt) override {
		owns = true;

		this->width	 = (int) w;
		this->height = (int) h;

		auto type = pixelFormatToGLType(fmt);
		glGenTextures(1, &textureID);

		glBindTexture(GL_TEXTURE_2D, textureID);

		GLuint internalFormat = GL_RGBA8;
		if (type == GL_RGB) {
			internalFormat = GL_RGB8;
		} else if (type == GL_RGBA) {
			internalFormat = GL_RGBA8;
		} else {
			mzAssert(0);
		}
		// Non-RGBA rows aren't guaranteed to be 4-aligned. Intel iGPU drivers
		// overread the buffer in glTexImage2D when alignment is the default 4
		// and the row stride isn't a multiple of 4 — drop to 1 across the call.
		// https://stackoverflow.com/questions/58925604/glteximage2d-crashing-program
		GLint prevUnpackAlignment = 4;
		if (type != GL_RGBA) {
			glGetIntegerv(GL_UNPACK_ALIGNMENT, &prevUnpackAlignment);
			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		}

		glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, w, h, 0, type, GL_UNSIGNED_BYTE, data);

		if (type != GL_RGBA) {
			glPixelStorei(GL_UNPACK_ALIGNMENT, prevUnpackAlignment);
		}
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		glBindTexture(GL_TEXTURE_2D, 0);
	}
};
