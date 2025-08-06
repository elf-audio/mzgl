#pragma once
#include "GraphicsAPI.h"

#include "log.h"
#include "sokol_gfx.h"
#include "SokolShaderRegistry.h"
#include "SokolSamplerRegistry.h"

class SokolAPI : public GraphicsAPI {
public:
	SokolAPI(Graphics &g)
		: GraphicsAPI(g) {}

	void init() override { loadDefaultShaders(); }

	void setBoundTexture(sg_image boundTex, sg_sampler sampler) {
		boundTexture = boundTex;
		boundSampler = sampler;
	}

	sg_image getBoundTexture() { return boundTexture; }
	sg_sampler getSampler() { return boundSampler; }

	sg_sampler createOrReuseSampler(sg_sampler_desc desc) { return samplerRegistry.createOrReuseSampler(desc); }

	void setBlending(bool shouldBlend) override {
		// doesn't need to do anything
	}

	void setBlendMode(Graphics::BlendMode blendMode) override {
		// doesn't need to do anything
	}
	Graphics::BlendMode getBlendMode() { return Graphics::BlendMode::Alpha; }

	void clear(vec4 c) override;

	void maskOn(const Rectf &r) override;
	void maskOff() override;
	[[nodiscard]] bool isMaskOn() const override { return maskIsOn; }

	[[nodiscard]] Rectf getMaskRect() const override { return maskRect; }

	void readScreenPixels(std::vector<uint8_t> &outData, const Rectf &r) override {}
	void drawVerts(const std::vector<glm::vec2> &verts, Vbo::PrimitiveType type) override {
		auto vbo = Vbo::create();
		vbo->setVertices(verts);
		vbo->draw(g, type);
	}

	void drawVerts(const std::vector<glm::vec2> &verts,
				   const std::vector<glm::vec4> &cols,
				   Vbo::PrimitiveType type) override {}
	std::shared_ptr<ShaderDef> getShaderDef(const std::string &name) const {
		return shaderRegistry.getShaderDef(name);
	}

	SokolShaderRegistry &getShaderRegistry() { return shaderRegistry; }

private:
	static ShaderRef createDefaultShader(const std::string &name, Graphics *g, SokolAPI *api) {
		auto shader				= Shader::create(*g, name);
		shader->isDefaultShader = true;
		return shader;
	}
	SokolShaderRegistry shaderRegistry;
	SokolSamplerRegistry samplerRegistry;

	void loadDefaultShaders();

	bool maskIsOn = false;
	Rectf maskRect;

	sg_image boundTexture {0};
	sg_sampler boundSampler {0};
};
