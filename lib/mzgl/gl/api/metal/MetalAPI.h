#pragma once

// Native Metal backend. Consumes the same sokol-shdc-compiled shaders as the
// Sokol backend (the generated *.glsl.h headers carry metal_macos/metal_ios
// MSL source plus reflection), but renders through Metal directly - no
// sokol_gfx at runtime. sokol_gfx.h is included for its TYPES only (the
// shader registry reflects sokol-shdc output); none of its functions are
// linked in a Metal build.
//
// This header is pure C++ (it's included from the plain-C++ facade files via
// GraphicsBackendTypes.h); all Objective-C lives in the api/metal/*.mm files
// behind MetalAPIImpl.

#include "GraphicsAPI.h"

#include "log.h"
#include "sokol_gfx.h"
#include "SokolShaderRegistry.h"
#include <memory>

// MSAA sample count - the MTKView and every pipeline must agree on this.
constexpr int mzglMetalSampleCount = 4;

struct MetalAPIImpl;

class MetalAPI : public GraphicsAPI {
public:
	MetalAPI(Graphics &g);
	~MetalAPI() override;

	void init() override;

	[[nodiscard]] std::string getBackendName() const override { return "Metal"; }

	// blend state is baked into the pipelines (same as the Sokol backend) -
	// Graphics keeps the current mode and MetalShader reads it at draw time
	void setBlending(bool shouldBlend) override {}
	void setBlendMode(Graphics::BlendMode blendMode) override {}

	void clear(vec4 c) override;

	void maskOn(const Rectf &r) override;
	void maskOff() override;
	[[nodiscard]] bool isMaskOn() const override { return maskIsOn; }
	[[nodiscard]] Rectf getMaskRect() const override { return maskRect; }

	// screenshots go through Graphics::deferredSaveScreen installed by the
	// view (same scheme as the Sokol backend)
	void readScreenPixels(std::vector<uint8_t> &outData, const Rectf &r) override {}

	void drawVerts(const std::vector<glm::vec2> &verts, Vbo::PrimitiveType type) override;

	void drawVerts(const std::vector<glm::vec2> &verts,
				   const std::vector<glm::vec4> &cols,
				   Vbo::PrimitiveType type) override {}

	[[nodiscard]] std::shared_ptr<ShaderDef> getShaderDef(const std::string &name) const {
		return shaderRegistry.getShaderDef(name);
	}
	SokolShaderRegistry &getShaderRegistry() { return shaderRegistry; }

	// ---- frame hooks, called by the MTKView draw loop --------------------
	// mtkView is a (__bridge void *) MTKView. beginFrame creates the frame's
	// command buffer + render encoder from the view's current render pass
	// descriptor; endFrame ends encoding, presents and commits.
	void beginFrame(void *mtkView);
	void endFrame();
	// true between beginFrame and endFrame (draws outside a frame are dropped)
	[[nodiscard]] bool inFrame() const;

	// ---- texture binding state (see MetalTexture) ------------------------
	// texId is a handle into the process-global Metal texture registry;
	// samplerKey packs the sampler desc (filter/wrap bits, see MetalTexture).
	void setBoundTexture(uint32_t texId, uint32_t samplerKey) {
		boundTexture	 = texId;
		boundSamplerKey	 = samplerKey;
	}
	[[nodiscard]] uint32_t getBoundTexture() const { return boundTexture; }
	[[nodiscard]] uint32_t getBoundSamplerKey() const { return boundSamplerKey; }

	MetalAPIImpl &impl() { return *pimpl; }

private:
	void loadDefaultShaders();

	static ShaderRef createDefaultShader(const std::string &name, Graphics *g) {
		auto shader				= Shader::create(*g, name);
		shader->isDefaultShader = true;
		return shader;
	}

	SokolShaderRegistry shaderRegistry;
	std::unique_ptr<MetalAPIImpl> pimpl;

	bool maskIsOn = false;
	Rectf maskRect;

	uint32_t boundTexture	 = 0;
	uint32_t boundSamplerKey = 0;
};
