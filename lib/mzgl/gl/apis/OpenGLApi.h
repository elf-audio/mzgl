#pragma once

#include "GraphicsApi.h"
#include "mzOpenGL.h"
#include "glUtil.h"

class OpenGLApi : public GraphicsApi {
public:
	OpenGLApi(GraphicsState &state)
		: GraphicsApi(state) {}
	virtual ~OpenGLApi() {}
	void clear(const vec4 &bgColor) override {
		glClearColor(bgColor.r, bgColor.g, bgColor.b, bgColor.a);
		glClear(GL_COLOR_BUFFER_BIT);
	}

	void saveScreen(string pngPath, int width, int height) override {
		ImageRef img = Image::create(width, height, 4);

		glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, img->data.data());
		img->flipVertical();

		// for some reason on OSX there is a bit of alpha - this makes it fully opaque
		for (int i = 0; i < img->data.size(); i += 4) {
			img->data[i + 3] = 255;
		}
		img->save(pngPath);
	}

	GLint getDefaultFrameBufferId() { return defaultFBO; }

	void setBlendMode(BlendMode blendMode) override {
		if (blendMode == BlendMode::Additive) {
			glBlendFunc(GL_SRC_ALPHA, GL_ONE);
		} else {
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		}
	}

	void setBlending(bool shouldBlend) override {
		// blend mode is already the same, jump out.
		if (shouldBlend == state.blendingEnabled) return;

		state.blendingEnabled = shouldBlend;
		if (shouldBlend) {
			glEnable(GL_BLEND);
		} else {
			glDisable(GL_BLEND);
		}
	}

	void setStrokeWeight(float f) override {
		if (f != state.strokeWeight) {
			state.strokeWeight = f;
			glLineWidth(f);
		}
	}

	void maskOn(const Rectf &r) override {
		state.scissor = r;

		glEnable(GL_SCISSOR_TEST);
		glScissor(r.x, state.height - (r.bottom()), r.width, r.height);
	}

	void maskOff() override { glDisable(GL_SCISSOR_TEST); }

	bool isMaskOn() override { return glIsEnabled(GL_SCISSOR_TEST); }

	void initGraphics() override {
		glGetIntegerv(GL_FRAMEBUFFER_BINDING, &defaultFBO);

		const unsigned char *openglVersion = glGetString(GL_VERSION);
		if (openglVersion != nullptr) {
			Log::d() << "OpenGL Version: " << openglVersion;
#ifdef MZGL_GL2
			Log::d() << "MZGL Compiled with MZGL_GL2==TRUE";
#else
			Log::d() << "MZGL Compiled with MZGL_GL2==FALSE";
#endif
		} else {
			Log::d() << "OpenGL Version: null";
		}
		setBlending(true);

		// setBlending only works on change, so calling
		// glEnable(GL_BLEND) ensures state sync. It's a
		// bit sloppy but fixes android bugs for persistence
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		loadDefaultShaders();

#ifndef MZGL_GL2
		if (immediateVertexArray != 0) {
			Log::e()
				<< "Immediate vertex array recreated - is this bad? if on android, probs should clean this up";
		}
		glGenVertexArrays(1, &immediateVertexArray);
		GetError();

		glBindVertexArray(immediateVertexArray);
		GetError();
#endif
		if (state.immediateVertexBuffer != 0) {
			Log::e() << "Immediate vertex buffer recreated - is this bad?";
		}

		glGenBuffers(1, &state.immediateVertexBuffer);
		glGenBuffers(1, &state.immediateColorBuffer);
		glGenBuffers(1, &state.immediateIndexBuffer);
		GetError();
	}

private:
	void loadDefaultShaders() {
		// zero out all shaders before loading any, for android mostly,
		// as the context may have reset itself
		state.nothingShader	   = nullptr;
        state.colorShader		   = nullptr;
        state.colorTextureShader = nullptr;
        state.fontShader		   = nullptr;
        state.texShader		   = nullptr;

        state.nothingShader = Shader::create(state);

        state.nothingShader->loadFromString(STRINGIFY(uniform mat4 mvp;

												in vec4 Position;
												uniform lowp vec4 color;
												out lowp vec4 colorV;

												void main(void) {
													colorV		= color;
													gl_Position = mvp * Position;
												}

												),

									  STRINGIFY(

										  in lowp vec4 colorV; out vec4 fragColor;

										  void main(void) { fragColor = colorV; }

										  )

		);

        state.colorShader = Shader::create(state);
        state.colorShader->loadFromString(STRINGIFY(

										uniform mat4 mvp; uniform lowp vec4 color;

										in vec4 Position;
										in lowp vec4 Color;

										out lowp vec4 colorV;

										void main(void) {
											colorV		= Color * color;
											gl_Position = mvp * Position;
										}

										),

									STRINGIFY(

										in lowp vec4 colorV; out vec4 fragColor;

										void main(void) { fragColor = colorV; }

										)

		);

        state.colorTextureShader = Shader::create(state);
        state.colorTextureShader->loadFromString(
			STRINGIFY(

				uniform mat4 mvp;

				in vec4 Position;
				in lowp vec2 TexCoord;
				in lowp vec4 Color;

				out lowp vec4 colorV;
				out lowp vec2 texCoordV;
				void main(void) {
					colorV		= Color;
					texCoordV	= TexCoord;
					gl_Position = mvp * Position;
				}

				),
			STRINGIFY(

				in lowp vec4 colorV; in lowp vec2 texCoordV; out vec4 fragColor;
				uniform sampler2D myTextureSampler;

				void main(void) { fragColor = texture(myTextureSampler, texCoordV) * colorV; }

				)

		);

		// this is temporary - it is just like colorTextureShader, but divides the color by 255
		// TODO: need to write my own gl backend for fontstash
        state.fontShader = Shader::create(state);
        state.fontShader->loadFromString(
			STRINGIFY(

				uniform mat4 mvp;

				in vec4 Position;
				in vec2 TexCoord;

				uniform lowp vec4 color;

				out vec2 texCoordV;

				void main() {
					texCoordV	= TexCoord;
					gl_Position = mvp * Position;
				}

				),
			STRINGIFY(

				uniform lowp vec4 color; in vec2 texCoordV; out vec4 fragColor; uniform sampler2D myTextureSampler;

				void main() {
					fragColor = color;
					fragColor.a *= texture(myTextureSampler, texCoordV).a;
				}

				)

		);
        state.texShader = Shader::create(state);
        state.texShader->loadFromString(
			STRINGIFY(

				uniform mat4 mvp;

				in vec4 Position;
				in lowp vec2 TexCoord;
				uniform lowp vec4 color;

				out lowp vec4 colorV;
				out lowp vec2 texCoordV;
				void main(void) {
					colorV		= color;
					texCoordV	= TexCoord;
					gl_Position = mvp * Position;
				}

				),
			STRINGIFY(

				in lowp vec4 colorV; in lowp vec2 texCoordV; out vec4 fragColor;
				uniform sampler2D myTextureSampler;

				void main(void) { fragColor = texture(myTextureSampler, texCoordV) * colorV; }

				)

		);

        state.nothingShader->isDefaultShader		= true;
        state.colorShader->isDefaultShader		= true;
        state.colorTextureShader->isDefaultShader = true;
        state.texShader->isDefaultShader			= true;
        state.fontShader->isDefaultShader			= true;
	}

	int32_t defaultFBO;
};