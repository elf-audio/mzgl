

#include "SokolAPI.h"
#include "SokolDefaultShaders.h"

void SokolAPI::loadDefaultShaders() {
	registerShaders(shaderRegistry);
	g.nothingShader		 = createDefaultShader("nothing", &g, this);
	g.colorShader		 = createDefaultShader("color", &g, this);
	g.colorTextureShader = createDefaultShader("colorTexture", &g, this);
	g.texShader			 = createDefaultShader("tex", &g, this);
	g.fontShader		 = createDefaultShader("font", &g, this);
}

void SokolAPI::maskOn(const Rectf &r) {
	maskIsOn   = true;
	maskRect   = r;
	float mult = 1.f / g.pixelScale;
	sg_apply_scissor_rectf(r.x * mult, r.y * mult, r.width * mult, r.height * mult, true);
}
void SokolAPI::maskOff() {
	maskRect   = Rectf(0, 0, g.width, g.height);
	float mult = 1.f / g.pixelScale;

	sg_apply_scissor_rect(0, 0, g.width * mult, g.height * mult, true);
	maskIsOn = false;
}

void SokolAPI::clear(vec4 c) {
	auto vbo = Vbo::create();
	g.setBlending(false);
	g.setColor(c.r, c.g, c.b, c.a);
	vbo->setVertices({{0, 0}, {g.width, 0}, {g.width, g.height}, {0, g.height}});
	vbo->setIndices({0, 1, 2, 0, 2, 3});
	vbo->draw(g);
}