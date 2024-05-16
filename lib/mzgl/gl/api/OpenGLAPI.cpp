#include "OpenGLAPI.h"
#include "OpenGLDefaultShaders.h"
#include "log.h"
#include "mzOpenGL.h"

static int primitiveTypeToGLMode(Vbo::PrimitiveType mode) {
	switch (mode) {
		case Vbo::PrimitiveType::Triangles: return GL_TRIANGLES;
		case Vbo::PrimitiveType::TriangleStrip: return GL_TRIANGLE_STRIP;
		case Vbo::PrimitiveType::TriangleFan: return GL_TRIANGLE_FAN;
		case Vbo::PrimitiveType::LineLoop: return GL_LINE_LOOP;
		case Vbo::PrimitiveType::LineStrip: return GL_LINE_STRIP;
		case Vbo::PrimitiveType::Lines: return GL_LINES;

		default: {
			Log::e() << "ERROR!! invalid primitive type " << (int) mode;
			return GL_TRIANGLES;
		}
	}
}

int32_t OpenGLAPI::getDefaultFrameBufferId() {
	return defaultFBO;
}

void OpenGLAPI::init() {
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
	loadDefaultShaders();

#ifndef MZGL_GL2
	if (immediateVertexArray != 0) {
		Log::e() << "Immediate vertex array recreated - is this bad? if on android, probs should clean this up";
	}
	glGenVertexArrays(1, &immediateVertexArray);
	glBindVertexArray(immediateVertexArray);

#endif
	if (immediateVertexBuffer != 0) {
		Log::e() << "Immediate vertex buffer recreated - is this bad?";
	}

	glGenBuffers(1, &immediateVertexBuffer);
	glGenBuffers(1, &immediateColorBuffer);
	glGenBuffers(1, &immediateIndexBuffer);
}
static ShaderRef loadDefaultShader(Graphics &g, const std::string &vertSrc, const std::string &fragSrc) {
	auto shader = Shader::create(g);
	shader->loadFromString(vertSrc, fragSrc);
	shader->isDefaultShader = true;
	return shader;
}
void OpenGLAPI::loadDefaultShaders() {
	// zero out all shaders before loading any, for android mostly,
	// as the context may have reset itself
	g.nothingShader		 = nullptr;
	g.colorShader		 = nullptr;
	g.colorTextureShader = nullptr;
	g.fontShader		 = nullptr;
	g.texShader			 = nullptr;

	g.nothingShader		 = loadDefaultShader(g, nothingVertSrc, nothingFragSrc);
	g.colorShader		 = loadDefaultShader(g, colorVertSrc, colorFragSrc);
	g.colorTextureShader = loadDefaultShader(g, colorTextureVertSrc, colorTextureFragSrc);
	g.fontShader		 = loadDefaultShader(g, fontVertSrc, fontFragSrc);
	g.texShader			 = loadDefaultShader(g, texVertSrc, texFragSrc);
}

void OpenGLAPI::setLineWidth(float f) {
	glLineWidth(f);
}
void OpenGLAPI::setBlending(bool shouldBlend) {
	if (shouldBlend) {
		glEnable(GL_BLEND);
	} else {
		glDisable(GL_BLEND);
	}
}

void OpenGLAPI::drawVerts(const std::vector<glm::vec2> &verts, Vbo::PrimitiveType type) {
	g.nothingShader->begin();

	g.currShader->uniform("mvp", g.getMVP());
	if (g.currShader->needsColorUniform) {
		g.currShader->uniform("color", g.getColor());
	}

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

void OpenGLAPI::drawVerts(const std::vector<glm::vec2> &verts,
						  const std::vector<glm::vec4> &cols,
						  Vbo::PrimitiveType type) {
	g.colorShader->begin();

	g.currShader->uniform("mvp", g.getMVP());
	if (g.currShader->needsColorUniform) {
		g.currShader->uniform("color", g.getColor());
	}

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

void OpenGLAPI::maskOn(const Rectf &r) {
	glEnable(GL_SCISSOR_TEST);
	glScissor(r.x, g.height - (r.bottom()), r.width, r.height);
}

void OpenGLAPI::maskOff() {
	glDisable(GL_SCISSOR_TEST);
}

bool OpenGLAPI::isMaskOn() {
	return glIsEnabled(GL_SCISSOR_TEST);
}

Rectf OpenGLAPI::getMaskRect() {
	Rectf r;
	glGetFloatv(GL_SCISSOR_BOX, (float *) &r);

	r.y = g.height - r.y - r.height;
	return r;
}

void OpenGLAPI::readScreenPixels(uint8_t *data, const Rectf &r) {
	glReadPixels(r.x, r.y, r.width, r.height, GL_RGBA, GL_UNSIGNED_BYTE, data);
}
void OpenGLAPI::clear(vec4 c) {
	glClearColor(c.r, c.g, c.b, c.a);
	glClear(GL_COLOR_BUFFER_BIT);
}
void OpenGLAPI::setBlendMode(Graphics::BlendMode blendMode) {
	switch (blendMode) {
		case Graphics::BlendMode::Alpha: glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); break;
		case Graphics::BlendMode::Additive: glBlendFunc(GL_SRC_ALPHA, GL_ONE); break;
		case Graphics::BlendMode::Multiply: glBlendFunc(GL_DST_COLOR, GL_ONE_MINUS_SRC_ALPHA); break;
		case Graphics::BlendMode::Screen: glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_COLOR); break;
		case Graphics::BlendMode::Subtract: glBlendFunc(GL_ZERO, GL_ONE_MINUS_SRC_COLOR); break;
	}
}
