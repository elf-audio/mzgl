
#include "mzgl_platform.h"
CLANG_IGNORE_WARNINGS_BEGIN("-Wshorten-64-to-32")
CLANG_IGNORE_ADDITONAL_WARNING("-Wmacro-redefined")
#include "SokolAPI.h"
#include "SokolDefaultShaders.h"
CLANG_IGNORE_WARNINGS_END

// the shader registry of the active backend, for generated shader-registration
// code (customShaders.h) that must work on both Sokol and Metal
SokolShaderRegistry &mzglShaderRegistry(Graphics &g) {
	return static_cast<SokolAPI &>(g.getAPI()).getShaderRegistry();
}

void SokolAPI::loadDefaultShaders() {
	registerShaders(shaderRegistry);
	g.nothingShader		 = createDefaultShader("nothing", &g, this);
	g.colorShader		 = createDefaultShader("color", &g, this);
	g.colorTextureShader = createDefaultShader("colorTexture", &g, this);
	g.texShader			 = createDefaultShader("tex", &g, this);
	g.fontShader		 = createDefaultShader("font", &g, this);
	g.colorFontShader	 = createDefaultShader("colorFont", &g, this);
}

void SokolAPI::maskOn(const Rectf &r) {
	maskIsOn = true;
	maskRect = r;
	// g.width/g.height and the whole UI coordinate space are already in
	// framebuffer pixels (MacAppDelegate sets g.width = points * pixelScale,
	// matching the Metal drawableSize). sokol's scissor is also in framebuffer
	// pixels with origin_top_left=true, so the mask rect maps 1:1 - no scaling
	// and no y-flip. The old `* (1/pixelScale)` shrank it to the top-left
	// quarter on retina (harmless on Windows where pixelScale == 1).
	sg_apply_scissor_rectf(r.x, r.y, r.width, r.height, true);
}
void SokolAPI::maskOff() {
	maskRect = Rectf(0, 0, g.width, g.height);
	maskIsOn = false;
	sg_apply_scissor_rect(0, 0, g.width, g.height, true);
}

void SokolAPI::clear(vec4 c) {
	auto vbo = Vbo::createFromPool(g);
	// Draw the clear quad with blending off, but restore the caller's blend
	// state afterwards - otherwise every shape drawn after clear() loses
	// blending (real GL clear() doesn't touch blend state).
	const bool wasBlending = g.isBlending();
	g.setBlending(false);
	g.setColor(c.r, c.g, c.b, c.a);
	vbo->setVertices({{0, 0}, {g.width, 0}, {g.width, g.height}, {0, g.height}});
	vbo->setIndices({0, 1, 2, 0, 2, 3});
	vbo->draw(g);
	g.setBlending(wasBlending);
}

