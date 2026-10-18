#pragma once

// fontstash renderer for the native D3D11 backend - a one-to-one port of the
// sokol one's batching: the atlas is an R8 D3D11Texture (re-uploaded at most
// once per frame when glyphs were added) and each text run appends its
// interleaved x,y,u,v vertices to the backend's stream ring and draws
// immediately, so paint order is preserved exactly and no buffer objects are
// created per run. Included from Font.cpp (plain C++) - no COM here, the
// draw itself is D3D11API::drawTextRun.

#include "mzAssert.h"
#include "D3D11API.h"
#include "D3D11Texture.h"
#include "Graphics.h"
#include <vector>

#ifdef __cplusplus
extern "C" {
#endif

FONS_DEF FONScontext *d3d11FonsCreate(Graphics &g, int width, int height, int flags);
FONS_DEF void d3d11FonsDelete(FONScontext *ctx);

#ifdef __cplusplus
}
#endif

struct d3d11FONScontext {
	TextureRef texture;
	uint32_t textureId		= 0;
	int width				= 0;
	int height				= 0;
	Graphics *g				= nullptr;
	int lastFrameNumUpdated = -1;
	bool dirty				= false;
	unsigned char *data		= nullptr;
	// scratch for the interleaved vertex data of one run
	std::vector<float> xyuv;
};

static int d3d11Fons__renderCreate(void *userPtr, int width, int height) {
	d3d11FONScontext *ctx = (d3d11FONScontext *) userPtr;

	ctx->width	= width;
	ctx->height = height;

	auto tex = std::make_shared<D3D11Texture>(*ctx->g);
	tex->allocate(nullptr, width, height, Texture::PixelFormat::LUMINANCE);
	ctx->texture   = tex;
	ctx->textureId = tex->getId();
	return 1;
}

static int d3d11Fons__renderResize(void *userPtr, int width, int height) {
	// atlas grew - drop the old texture (the TextureRef owns it) and make a
	// bigger one
	d3d11FONScontext *ctx = (d3d11FONScontext *) userPtr;
	ctx->texture		  = nullptr;
	return d3d11Fons__renderCreate(userPtr, width, height);
}

static void d3d11Fons__renderUpdate(void *userPtr, int *rect, const unsigned char *data) {
	d3d11FONScontext *ctx = (d3d11FONScontext *) userPtr;
	ctx->data			  = (unsigned char *) data;
	ctx->dirty			  = true;
}

static void d3d11Fons__flushPendingAtlasUpdate(d3d11FONScontext *ctx) {
	Graphics &g = *ctx->g;
	if (ctx->texture == nullptr) return;
	auto *tex = static_cast<D3D11Texture *>(ctx->texture.get());
	// the device was replaced (device lost): the atlas texture is gone, but
	// fontstash keeps the atlas bitmap, so make a new one from it
	if (ctx->textureId != 0 && !mzglD3D11TextureValid(ctx->textureId) && ctx->data != nullptr) {
		tex->allocate(ctx->data, ctx->width, ctx->height, Texture::PixelFormat::LUMINANCE);
		ctx->textureId			 = tex->getId();
		ctx->dirty				 = false;
		ctx->lastFrameNumUpdated = g.getFrameNum();
		return;
	}
	if (ctx->dirty && ctx->lastFrameNumUpdated != (int) g.getFrameNum()) {
		ctx->lastFrameNumUpdated = g.getFrameNum();
		ctx->dirty				 = false;
		tex->updateData(ctx->data);
	}
}

static void d3d11Fons__renderDraw(void *userPtr, const float *verts, const float *tcoords, int nverts) {
	d3d11FONScontext *ctx = (d3d11FONScontext *) userPtr;
	Graphics &g			  = *ctx->g;
	if (ctx->texture == nullptr || nverts <= 0) return;

	d3d11Fons__flushPendingAtlasUpdate(ctx);

	// interleave x,y,u,v - one buffer slot 0, stride 16
	auto &d = ctx->xyuv;
	d.resize((size_t) nverts * 4);
	for (int i = 0; i < nverts; i++) {
		d[i * 4 + 0] = verts[i * 2 + 0];
		d[i * 4 + 1] = verts[i * 2 + 1];
		d[i * 4 + 2] = tcoords[i * 2 + 0];
		d[i * 4 + 3] = tcoords[i * 2 + 1];
	}

	ctx->texture->bind();
	static_cast<D3D11API *>(&g.getAPI())->drawTextRun(d.data(), nverts);
	ctx->texture->unbind();
}

static void d3d11Fons__renderDelete(void *userPtr) {
	d3d11FONScontext *ctx = (d3d11FONScontext *) userPtr;
	delete ctx;
}

FONS_DEF FONScontext *d3d11FonsCreate(Graphics &g, int width, int height, int flags) {
	FONSparams params;

	d3d11FONScontext *ctx = new d3d11FONScontext();
	ctx->g				  = &g;
	memset(&params, 0, sizeof(params));

	params.width		= width;
	params.height		= height;
	params.flags		= (unsigned char) flags;
	params.renderCreate = d3d11Fons__renderCreate;
	params.renderResize = d3d11Fons__renderResize;
	params.renderUpdate = d3d11Fons__renderUpdate;
	params.renderDraw	= d3d11Fons__renderDraw;
	params.renderDelete = d3d11Fons__renderDelete;
	params.userPtr		= ctx;

	return fonsCreateInternal(&params);
}

FONS_DEF void d3d11FonsDelete(FONScontext *ctx) {
	fonsDeleteInternal(ctx);
}
