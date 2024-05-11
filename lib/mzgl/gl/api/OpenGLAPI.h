#pragma once

#include "GraphicsAPI.h"
#include "mzOpenGL.h"
#include "glUtil.h"
class OpenGLAPI : public GraphicsAPI {
public:
	OpenGLAPI(Graphics &g)
		: GraphicsAPI(g) {}

	void init() override {
		glGetIntegerv(GL_FRAMEBUFFER_BINDING, &g.defaultFBO);

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
		if (immediateVertexBuffer != 0) {
			Log::e() << "Immediate vertex buffer recreated - is this bad?";
		}

		glGenBuffers(1, &immediateVertexBuffer);
		GetError();
		glGenBuffers(1, &immediateColorBuffer);
		GetError();
		glGenBuffers(1, &immediateIndexBuffer);
		GetError();
	}

	void loadDefaultShaders() {
		// zero out all shaders before loading any, for android mostly,
		// as the context may have reset itself
		g.nothingShader		 = nullptr;
		g.colorShader		 = nullptr;
		g.colorTextureShader = nullptr;
		g.fontShader		 = nullptr;
		g.texShader			 = nullptr;

		g.nothingShader = Shader::create(g);

		g.nothingShader->loadFromString(STRINGIFY(uniform mat4 mvp;

												  in vec4 Position;
												  uniform lowp vec4 color;
												  out lowp vec4 colorV;

												  void main(void) {
													  colorV	  = color;
													  gl_Position = mvp * Position;
												  }

												  ),

										STRINGIFY(

											in lowp vec4 colorV; out vec4 fragColor;

											void main(void) { fragColor = colorV; }

											)

		);

		g.colorShader = Shader::create(g);
		g.colorShader->loadFromString(STRINGIFY(

										  uniform mat4 mvp; uniform lowp vec4 color;

										  in vec4 Position;
										  in lowp vec4 Color;

										  out lowp vec4 colorV;

										  void main(void) {
											  colorV	  = Color * color;
											  gl_Position = mvp * Position;
										  }

										  ),

									  STRINGIFY(

										  in lowp vec4 colorV; out vec4 fragColor;

										  void main(void) { fragColor = colorV; }

										  )

		);

		g.colorTextureShader = Shader::create(g);
		g.colorTextureShader->loadFromString(
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
		g.fontShader = Shader::create(g);
		g.fontShader->loadFromString(
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
		g.texShader = Shader::create(g);
		g.texShader->loadFromString(
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

		g.nothingShader->isDefaultShader	  = true;
		g.colorShader->isDefaultShader		  = true;
		g.colorTextureShader->isDefaultShader = true;
		g.texShader->isDefaultShader		  = true;
		g.fontShader->isDefaultShader		  = true;
	}

	void setLineWidth(float f) override { glLineWidth(f); }
	void setBlending(bool shouldBlend) override {
		if (shouldBlend) {
			glEnable(GL_BLEND);
		} else {
			glDisable(GL_BLEND);
		}
	}

	void drawVerts(const std::vector<glm::vec2> &verts, Vbo::PrimitiveType type) override {
		g.nothingShader->begin();

		g.currShader->uniform("mvp", g.getMVP());
		if (g.currShader->needsColorUniform) {
			g.currShader->uniform("color", g.getColor());
		}

		// now draw

#ifndef MZGL_GL2
		if (immediateVertexArray == 0) return;
		glBindVertexArray(immediateVertexArray);
#endif

		glEnableVertexAttribArray(g.currShader->positionAttribute);
		glBindBuffer(GL_ARRAY_BUFFER, immediateVertexBuffer);
		glBufferData(GL_ARRAY_BUFFER, verts.size() * 2 * sizeof(float), verts.data(), GL_DYNAMIC_DRAW);
		glVertexAttribPointer(g.currShader->positionAttribute, 2, GL_FLOAT, GL_FALSE, 0, NULL);

		auto glType = primitiveTypeToGLMode(type);

		glDrawArrays(glType, 0, (GLsizei) verts.size());

		glDisableVertexAttribArray(g.currShader->positionAttribute);

#ifndef MZGL_GL2
		glBindVertexArray(0);
#endif
	}

	void drawVerts(const std::vector<glm::vec2> &verts,
				   const std::vector<glm::vec4> &cols,
				   Vbo::PrimitiveType type) override {
		g.colorShader->begin();

		g.currShader->uniform("mvp", g.getMVP());
		if (g.currShader->needsColorUniform) {
			g.currShader->uniform("color", g.getColor());
		}

		// now draw

#ifndef MZGL_GL2
		if (immediateVertexArray == 0) return;
		glBindVertexArray(immediateVertexArray);
#endif

		glEnableVertexAttribArray(g.currShader->positionAttribute);
		glBindBuffer(GL_ARRAY_BUFFER, immediateVertexBuffer);
		glBufferData(GL_ARRAY_BUFFER, verts.size() * 2 * sizeof(float), verts.data(), GL_DYNAMIC_DRAW);
		glVertexAttribPointer(g.currShader->positionAttribute, 2, GL_FLOAT, GL_FALSE, 0, NULL);

		glEnableVertexAttribArray(g.currShader->colorAttribute);
		glBindBuffer(GL_ARRAY_BUFFER, immediateColorBuffer);
		glBufferData(GL_ARRAY_BUFFER, cols.size() * 4 * sizeof(float), cols.data(), GL_DYNAMIC_DRAW);
		glVertexAttribPointer(g.currShader->colorAttribute, 4, GL_FLOAT, GL_FALSE, 0, NULL);

		glDrawArrays(primitiveTypeToGLMode(type), 0, (GLsizei) verts.size());

		glDisableVertexAttribArray(g.currShader->positionAttribute);
		glDisableVertexAttribArray(g.currShader->colorAttribute);

#ifndef MZGL_GL2
		glBindVertexArray(0);
#endif
	}

	void drawVerts(const std::vector<glm::vec2> &verts, const std::vector<uint32_t> &indices) override {
		//	VboRef vbo = Vbo::create();
		//	vbo->setVertices(verts);
		//	vbo->setIndices(indices);
		//	vbo->draw(*this);

		g.nothingShader->begin();
		g.currShader->uniform("mvp", g.getMVP());
		if (g.currShader->needsColorUniform) {
			g.currShader->uniform("color", g.getColor());
		}
		// now draw

		Log::e() << "drawVerts(verts, indices) has changed and is not tested";

#ifndef MZGL_GL2
		if (immediateVertexArray == 0) return;
		glBindVertexArray(immediateVertexArray);
#endif

		glEnableVertexAttribArray(g.currShader->positionAttribute);
		glBindBuffer(GL_ARRAY_BUFFER, immediateVertexBuffer);
		glBufferData(GL_ARRAY_BUFFER, verts.size() * 2 * sizeof(float), verts.data(), GL_DYNAMIC_DRAW);
		glVertexAttribPointer(g.currShader->positionAttribute, 2, GL_FLOAT, GL_FALSE, 0, NULL);

		//	glEnableVertexAttribArray(currShader->);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, immediateColorBuffer);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint32_t), indices.data(), GL_DYNAMIC_DRAW);
		//	glVertexAttribPointer(currShader->colorAttribute, 4, GL_FLOAT, GL_FALSE, 0, NULL);

		glDrawElements(GL_TRIANGLES, (GLsizei) indices.size(), GL_UNSIGNED_INT, 0);
		//	glDrawArrays(type, 0, verts.size());

		glDisableVertexAttribArray(g.currShader->positionAttribute);
		//	glDisableVertexAttribArray(currShader->colorAttribute);

#ifndef MZGL_GL2
		glBindVertexArray(0);
#endif
	}

	void maskOn(const Rectf &r) override {
		glEnable(GL_SCISSOR_TEST);
		glScissor(r.x, g.height - (r.bottom()), r.width, r.height);
	}

	void maskOff() override { glDisable(GL_SCISSOR_TEST); }

	bool isMaskOn() override { return glIsEnabled(GL_SCISSOR_TEST); }

	Rectf getMaskRect() override {
		Rectf r;
		glGetFloatv(GL_SCISSOR_BOX, (float *) &r);

		r.y = g.height - r.y - r.height;
		return r;
	}

	void readScreenPixels(uint8_t *data, const Rectf &r) override {
		glReadPixels(r.x, r.y, r.width, r.height, GL_RGBA, GL_UNSIGNED_BYTE, data);
	}
	void clear(vec4 c) override {
		glClearColor(c.r, c.g, c.b, c.a);
		glClear(GL_COLOR_BUFFER_BIT);
	}
	void setBlendMode(Graphics::BlendMode blendMode) override {
		switch (blendMode) {
			case Graphics::BlendMode::Alpha: glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); break;
			case Graphics::BlendMode::Additive: glBlendFunc(GL_SRC_ALPHA, GL_ONE); break;
			case Graphics::BlendMode::Multiply: glBlendFunc(GL_DST_COLOR, GL_ONE_MINUS_SRC_ALPHA); break;
			case Graphics::BlendMode::Screen: glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_COLOR); break;
			case Graphics::BlendMode::Subtract: glBlendFunc(GL_ZERO, GL_ONE_MINUS_SRC_COLOR); break;
		}
	}

private:
	uint32_t immediateVertexArray  = 0;
	uint32_t immediateVertexBuffer = 0;
	uint32_t immediateColorBuffer  = 0;
	uint32_t immediateIndexBuffer  = 0;
};
