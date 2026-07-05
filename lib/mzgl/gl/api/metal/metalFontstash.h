#pragma once

// fontstash renderer for the native Metal backend. Unlike the sokol one this
// goes through the backend-agnostic Texture/Vbo interfaces: the atlas is an
// R8 MetalTexture (updated with replaceRegion when glyphs are added) and each
// text run draws a transient pos+uv Vbo with the current font shader.
// Included from Font.cpp (plain C++) - no Objective-C here.

#include "mzAssert.h"
#include "MetalTexture.h"
#include "Vbo.h"
#include "Graphics.h"

#ifdef __cplusplus
extern "C" {
#endif

FONS_DEF FONScontext *metalFonsCreate(Graphics &g, int width, int height, int flags);
FONS_DEF void metalFonsDelete(FONScontext *ctx);

#ifdef __cplusplus
}
#endif

struct metalFONScontext {
	TextureRef texture;
	uint32_t textureId = 0;
	int width		   = 0;
	int height		   = 0;
	Graphics *g		   = nullptr;
	int lastFrameNumUpdated = -1;
	bool dirty				= false;
	unsigned char *data		= nullptr;
	// one Vbo reused for every text run (its CPU buffers are just refilled;
	// small runs go out via setVertexBytes so no GPU buffer churn either)
	VboRef vbo;
	std::vector<glm::vec2> verts;
	std::vector<glm::vec2> texCoords;
};

static int metalFons__renderCreate(void *userPtr, int width, int height) {
	metalFONScontext *ctx = (metalFONScontext *) userPtr;

	ctx->width	= width;
	ctx->height = height;

	auto tex = std::make_shared<MetalTexture>(*ctx->g);
	tex->allocate(nullptr, width, height, Texture::PixelFormat::LUMINANCE);
	ctx->texture   = tex;
	ctx->textureId = tex->getId();
	return 1;
}

static int metalFons__renderResize(void *userPtr, int width, int height) {
	// atlas grew - drop the old texture (the TextureRef owns it) and make a
	// bigger one
	metalFONScontext *ctx = (metalFONScontext *) userPtr;
	ctx->texture		  = nullptr;
	return metalFons__renderCreate(userPtr, width, height);
}

static void metalFons__renderUpdate(void *userPtr, int *rect, const unsigned char *data) {
	metalFONScontext *ctx = (metalFONScontext *) userPtr;
	ctx->data			  = (unsigned char *) data;
	ctx->dirty			  = true;
}

static void metalFons__renderDraw(void *userPtr, const float *verts, const float *tcoords, int nverts) {
	metalFONScontext *ctx = (metalFONScontext *) userPtr;
	Graphics &g			  = *ctx->g;
	if (ctx->texture == nullptr) return;

	if (ctx->dirty && ctx->lastFrameNumUpdated != g.getFrameNum()) {
		ctx->lastFrameNumUpdated = g.getFrameNum();
		ctx->dirty				 = false;
		static_cast<MetalTexture *>(ctx->texture.get())->updateData(ctx->data);
	}

	ctx->texture->bind();

	auto &v = ctx->verts;
	auto &t = ctx->texCoords;
	v.clear();
	t.clear();
	v.reserve(nverts);
	t.reserve(nverts);
	for (int i = 0; i < nverts; i++) {
		v.emplace_back(verts[i * 2], verts[i * 2 + 1]);
		t.emplace_back(tcoords[i * 2], tcoords[i * 2 + 1]);
	}

	if (ctx->vbo == nullptr) ctx->vbo = Vbo::create();
	ctx->vbo->setVertices(v);
	ctx->vbo->setTexCoords(t);
	ctx->vbo->draw(g);

	ctx->texture->unbind();
}

static void metalFons__renderDelete(void *userPtr) {
	metalFONScontext *ctx = (metalFONScontext *) userPtr;
	delete ctx;
}

FONS_DEF FONScontext *metalFonsCreate(Graphics &g, int width, int height, int flags) {
	FONSparams params;

	metalFONScontext *ctx = new metalFONScontext();
	ctx->g				  = &g;
	memset(&params, 0, sizeof(params));

	params.width		= width;
	params.height		= height;
	params.flags		= (unsigned char) flags;
	params.renderCreate = metalFons__renderCreate;
	params.renderResize = metalFons__renderResize;
	params.renderUpdate = metalFons__renderUpdate;
	params.renderDraw	= metalFons__renderDraw;
	params.renderDelete = metalFons__renderDelete;
	params.userPtr		= ctx;

	return fonsCreateInternal(&params);
}

FONS_DEF void metalFonsDelete(FONScontext *ctx) {
	fonsDeleteInternal(ctx);
}
