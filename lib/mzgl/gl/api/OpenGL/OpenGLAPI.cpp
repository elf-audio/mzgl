#include "OpenGLAPI.h"
#include "OpenGLShader.h"
#include "OpenGLDefaultShaders.h"
#include "log.h"
#include "mzOpenGL.h"
#include "mzgl_platform.h"
#include "error.h"
static int primitiveTypeToGLMode(Vbo::PrimitiveType mode) {
	switch (mode) {
		case Vbo::PrimitiveType::Triangles: return GL_TRIANGLES;
		case Vbo::PrimitiveType::TriangleStrip: return GL_TRIANGLE_STRIP;
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
	GetError();
	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &defaultFBO);
	GetError();
	const unsigned char *openglVersion = glGetString(GL_VERSION);
	if (openglVersion != nullptr) {
		Log::d() << "OpenGL Version: " << openglVersion;
	} else {
		Log::d() << "OpenGL Version: null";
	}
	GetError();
	loadDefaultShaders();
	GetError();
	if (immediateVertexArray != 0) {
		Log::e() << "Immediate vertex array recreated - is this bad? if on android, probs should clean this up";
	}
	GetError();
	glGenVertexArrays(1, &immediateVertexArray);
	GetError();
	glBindVertexArray(immediateVertexArray);
	GetError();
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

void OpenGLAPI::setBlending(bool shouldBlend) {
	GetError();
	if (shouldBlend) {
		glEnable(GL_BLEND);
	} else {
		glDisable(GL_BLEND);
	}
	GetError();
}

void OpenGLAPI::drawVerts(const std::vector<glm::vec2> &verts, Vbo::PrimitiveType type) {
	GetError();
	auto shader = dynamic_cast<OpenGLShader *>(g.nothingShader.get());
	shader->begin();
	GetError();
	shader->setMVP(g.getMVP());
	shader->setColor(g.getColor());
	GetError();
	if (immediateVertexArray == 0) return;
	glBindVertexArray(immediateVertexArray);
	GetError();
	glEnableVertexAttribArray(shader->positionAttribute);
	GetError();
	glBindBuffer(GL_ARRAY_BUFFER, immediateVertexBuffer);
	GetError();
	glBufferData(GL_ARRAY_BUFFER, verts.size() * 2 * sizeof(float), verts.data(), GL_DYNAMIC_DRAW);
	GetError();
	glVertexAttribPointer(shader->positionAttribute, 2, GL_FLOAT, GL_FALSE, 0, NULL);
	GetError();
	auto glType = primitiveTypeToGLMode(type);

	glDrawArrays(glType, 0, (GLsizei) verts.size());
	GetError();
	glDisableVertexAttribArray(shader->positionAttribute);
	GetError();
	glBindVertexArray(0);
	GetError();
}

void OpenGLAPI::drawVerts(const std::vector<glm::vec2> &verts,
						  const std::vector<glm::vec4> &cols,
						  Vbo::PrimitiveType type) {
	auto shader = dynamic_cast<OpenGLShader *>(g.colorShader.get());
	GetError();
	shader->begin();
	GetError();
	shader->setMVP(g.getMVP());
	GetError();
	shader->setColor(g.getColor());
	GetError();
	if (immediateVertexArray == 0) return;
	GetError();
	glBindVertexArray(immediateVertexArray);
	GetError();
	glEnableVertexAttribArray(shader->positionAttribute);
	GetError();
	glBindBuffer(GL_ARRAY_BUFFER, immediateVertexBuffer);
	GetError();
	glBufferData(GL_ARRAY_BUFFER, verts.size() * 2 * sizeof(float), verts.data(), GL_DYNAMIC_DRAW);
	GetError();
	glVertexAttribPointer(shader->positionAttribute, 2, GL_FLOAT, GL_FALSE, 0, NULL);
	GetError();
	glEnableVertexAttribArray(shader->colorAttribute);
	GetError();
	glBindBuffer(GL_ARRAY_BUFFER, immediateColorBuffer);
	GetError();
	glBufferData(GL_ARRAY_BUFFER, cols.size() * 4 * sizeof(float), cols.data(), GL_DYNAMIC_DRAW);
	GetError();
	glVertexAttribPointer(shader->colorAttribute, 4, GL_FLOAT, GL_FALSE, 0, NULL);
	GetError();
	glDrawArrays(primitiveTypeToGLMode(type), 0, (GLsizei) verts.size());
	GetError();
	glDisableVertexAttribArray(shader->positionAttribute);
	GetError();
	glDisableVertexAttribArray(shader->colorAttribute);
	GetError();
	glBindVertexArray(0);
}

void OpenGLAPI::maskOn(const Rectf &r) {
	GetError();
	glEnable(GL_SCISSOR_TEST);
	GetError();
#if MZGL_MAC
	if (g.pixelScale == 1.f) {
		glScissor(r.x * 0.5f, (g.height - (r.bottom())) * 0.5f, r.width * 0.5f, r.height * 0.5f);
		GetError();
		return;
	}
#endif
	glScissor(r.x, g.height - (r.bottom()), r.width, r.height);
	GetError();
}

void OpenGLAPI::maskOff() {
	GetError();
	glDisable(GL_SCISSOR_TEST);
}

bool OpenGLAPI::isMaskOn() const {
	GetError();
	return glIsEnabled(GL_SCISSOR_TEST);
}

Rectf OpenGLAPI::getMaskRect() const {
	Rectf r;
	GetError();
	glGetFloatv(GL_SCISSOR_BOX, (float *) &r);
	GetError();
	r.y = g.height - r.y - r.height;
	return r;
}

void OpenGLAPI::readScreenPixels(std::vector<uint8_t> &outData, const Rectf &r) {
	GetError();
	outData.resize(r.width * r.height * 4);
	GetError();
	glReadPixels(r.x, r.y, r.width, r.height, GL_RGBA, GL_UNSIGNED_BYTE, outData.data());
	GetError();
}

void OpenGLAPI::clear(vec4 c) {
	GetError();
	glClearColor(c.r, c.g, c.b, c.a);
	GetError();
	glClear(GL_COLOR_BUFFER_BIT);
	GetError();
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
