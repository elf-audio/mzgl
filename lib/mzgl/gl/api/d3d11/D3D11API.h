#pragma once

// Native Direct3D 11 backend (Windows). Consumes the same sokol-shdc-compiled
// shaders as the Sokol backend (the generated *.glsl.h headers carry hlsl5
// source plus reflection) but renders through D3D11 directly: no sokol_gfx at
// runtime and no process-global fixed-size resource pools. sokol_gfx.h is
// included for its TYPES only (the shader registry reflects sokol-shdc
// output); none of its functions are linked in a D3D11 build.
//
// Pure C++ header (it's included from the plain-C++ facade files via
// GraphicsBackendTypes.h); the COM side lives in api/d3d11/*.cpp behind
// D3D11APIImpl and D3D11Internal.h.

#include "GraphicsAPI.h"

#include "log.h"
#include "sokol_gfx.h"
#include "SokolShaderRegistry.h"
#include <memory>
#include <string>

// MSAA sample count - the D3D11Context render target and the rasterizer state
// must agree on this.
constexpr int mzglD3D11SampleCount = 4;

class D3D11Context;
struct D3D11APIImpl;

class D3D11API : public GraphicsAPI {
public:
	D3D11API(Graphics &g);
	~D3D11API() override;

	void init() override;

	[[nodiscard]] std::string getBackendName() const override { return "D3D11"; }

	// blend state is resolved at draw time from Graphics (same as the Sokol
	// and Metal backends)
	void setBlending(bool shouldBlend) override {}
	void setBlendMode(Graphics::BlendMode blendMode) override {}

	void clear(vec4 c) override;

	void maskOn(const Rectf &r) override;
	void maskOff() override;
	[[nodiscard]] bool isMaskOn() const override { return maskIsOn; }
	[[nodiscard]] Rectf getMaskRect() const override { return maskRect; }

	// Synchronous readback of what has been drawn so far this frame (the MSAA
	// target is resolved into a staging copy). Rows come back bottom-up like
	// glReadPixels, so Graphics::saveScreen's flip produces an upright image.
	// Graphics::saveScreen normally goes through deferredSaveScreen instead
	// (installed by init()), which captures the finished frame in endFrame.
	void readScreenPixels(std::vector<uint8_t> &outData, const Rectf &r) override;

	void drawVerts(const std::vector<glm::vec2> &verts, Vbo::PrimitiveType type) override;

	void drawVerts(const std::vector<glm::vec2> &verts,
				   const std::vector<glm::vec4> &cols,
				   Vbo::PrimitiveType type) override;

	[[nodiscard]] std::shared_ptr<ShaderDef> getShaderDef(const std::string &name) const {
		return shaderRegistry.getShaderDef(name);
	}
	SokolShaderRegistry &getShaderRegistry() { return shaderRegistry; }

	// ---- frame hooks, called by the window's frame loop --------------------
	// beginFrame binds the context's (MSAA) render target, clears it to black
	// and resets the per-frame state. endFrame resolves MSAA into the back
	// buffer, writes a pending screenshot and presents. endFrame returns false
	// when the device was lost (the reason is logged); the caller then
	// recreates its D3D11Context and calls mzglD3D11::setDevice with the new
	// device.
	void beginFrame(D3D11Context &ctx);
	bool endFrame(bool vsync);
	// true between beginFrame and endFrame (draws outside a frame are dropped)
	[[nodiscard]] bool inFrame() const;

	// ---- texture binding state (see D3D11Texture) -------------------------
	// texId is a handle into the process-global texture registry; samplerKey
	// packs the sampler desc (filter/wrap bits, see D3D11Texture).
	void setBoundTexture(uint32_t texId, uint32_t samplerKey) {
		boundTexture	= texId;
		boundSamplerKey = samplerKey;
	}
	[[nodiscard]] uint32_t getBoundTexture() const { return boundTexture; }
	[[nodiscard]] uint32_t getBoundSamplerKey() const { return boundSamplerKey; }

	// fontstash: stream `nverts` interleaved x,y,u,v vertices (stride 16) and
	// draw them as triangles with the font/tex shader, the bound texture being
	// the atlas - the one-to-one port of sokolFontstash's append+draw
	void drawTextRun(const float *xyuv, int nverts);

	D3D11APIImpl &impl() { return *pimpl; }

private:
	void loadDefaultShaders();

	static ShaderRef createDefaultShader(const std::string &name, Graphics *g) {
		auto shader				= Shader::create(*g, name);
		shader->isDefaultShader = true;
		return shader;
	}

	SokolShaderRegistry shaderRegistry;
	std::unique_ptr<D3D11APIImpl> pimpl;

	bool maskIsOn = false;
	Rectf maskRect;

	uint32_t boundTexture	 = 0;
	uint32_t boundSamplerKey = 0;
};

// true while the registry texture behind `id` exists on the current device
// (false after a device loss); pure C++ accessor for the fontstash
bool mzglD3D11TextureValid(uint32_t id);

// one-line report of the backend's live GPU resources (buffers, textures,
// stream ring usage) for the test server's dump-gpu-buffers action
std::string mzglD3D11ResourceReport();
