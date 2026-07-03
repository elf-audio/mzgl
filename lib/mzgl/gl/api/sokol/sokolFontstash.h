
#pragma once
//#include "log.h"
#include "sokol_gfx.h"
#include "mzAssert.h"
#include "SokolAPI.h"
#include "SokolShader.h"
#include "SokolPipeline.h"
#include "SokolBufferTracker.h"
#ifdef __cplusplus
extern "C" {
#endif

FONS_DEF FONScontext *sokolFonsCreate(Graphics &g, int width, int height, int flags);
FONS_DEF void sokolFonsDelete(FONScontext *ctx);

#ifdef __cplusplus
}
#endif

struct sokolFONScontext {
	TextureRef texture;
	sg_image tex;
	int width  = 0;
	int height = 0;
	Graphics *g;
	int lastFrameNumUpdated = -1;
	bool dirty				= false;
	unsigned char *data		= nullptr;

	// One shared stream buffer for all text draws: each draw appends its
	// vertices (sg_append_buffer is legal many times per frame, unlike
	// sg_update_buffer) and issues its sg_draw immediately with a byte offset,
	// preserving paint order exactly. This replaces one pooled buffer pair per
	// text run (~200 sokol buffer slots on text-heavy screens) with one slot.
	sg_buffer appendBuf = {SG_INVALID_ID};
};

// Worst-case measured text data on the busiest screen is ~25 KB/frame, so
// 256 KB is ~10x headroom; overflow falls back to the old pooled-VBO path anyway.
static constexpr int sokolFonsAppendBufSize = 256 * 1024;

static int sokolFons__renderCreate(void *userPtr, int width, int height) {
	sokolFONScontext *gl = (sokolFONScontext *) userPtr;

	gl->width  = width;
	gl->height = height;

	mzAssert(gl->tex.id == SG_INVALID_ID);
	sg_image_desc img_desc;
	memset(&img_desc, 0, sizeof(img_desc));
	img_desc.width		  = gl->width;
	img_desc.height		  = gl->height;
	img_desc.usage		  = SG_USAGE_DYNAMIC;
	img_desc.pixel_format = SG_PIXELFORMAT_R8;
	gl->tex				  = sg_make_image(&img_desc);
	gl->texture			  = Texture::create(*gl->g, gl->tex.id, gl->width, gl->height);

	return 1;
}

static int sokolFons__renderResize(void *userPtr, int width, int height) {
	sokolFONScontext *gl = (sokolFONScontext *) userPtr;
	// Atlas grew: fontstash asks us to make a bigger texture. The old image is
	// still live here - destroy it first, otherwise we leak one sokol image slot
	// per atlas growth (512->1024->2048...). The wrapper TextureRef we made for it
	// has owns=false so it won't free it either. Leaked slots eventually exhaust
	// the image pool -> sg_make_image returns SG_INVALID_ID -> glyphs/icons bind a
	// dead texture and render as solid (black) rects. renderCreate asserts the
	// handle is invalid, so we must clear it before delegating.
	if (gl->tex.id != SG_INVALID_ID && sg_isvalid()) {
		sg_destroy_image(gl->tex);
	}
	gl->tex = sg_image {SG_INVALID_ID};
	return sokolFons__renderCreate(userPtr, width, height);
}

static void sokolFons__renderUpdate(void *userPtr, int *rect, const unsigned char *data) {
	sokolFONScontext *gl = (sokolFONScontext *) userPtr;

	gl->data  = (unsigned char *) data;
	gl->dirty = true;
}

// Fallback: one pooled VBO per text run (the pre-append behaviour). Used only
// if the append buffer would overflow this frame.
static void sokolFons__renderDrawPooled(sokolFONScontext *sfons,
										const float *verts,
										const float *tcoords,
										int nverts) {
	Graphics &g = *sfons->g;
	std::vector<glm::vec2> v;
	v.reserve(nverts);
	for (int i = 0; i < nverts; i++) {
		v.push_back(glm::vec2(verts[i * 2], verts[i * 2 + 1]));
	}
	std::vector<glm::vec2> t;
	t.reserve(nverts);
	for (int i = 0; i < nverts; i++) {
		t.push_back(glm::vec2(tcoords[i * 2], tcoords[i * 2 + 1]));
	}

	auto vbo = Vbo::createFromPool(g);
	vbo->setVertices(v);
	vbo->setTexCoords(t);
	vbo->draw(g);
}

// Mirror of SokolVbo::getShader() for a pos+uv-only vertex layout.
static SokolShader *sokolFons__pickShader(Graphics &g) {
	if (g.currShader && !g.currShader->isDefaultShader) {
		return dynamic_cast<SokolShader *>(g.currShader);
	}
	if (g.currShader == g.fontShader.get()) {
		return dynamic_cast<SokolShader *>(g.fontShader.get());
	}
	return dynamic_cast<SokolShader *>(g.texShader.get());
}

static void sokolFons__renderDraw(void *userPtr, const float *verts, const float *tcoords, int nverts) {
	sokolFONScontext *sfons = (sokolFONScontext *) userPtr;
	Graphics &g				= *sfons->g;

	if (sfons->dirty && sfons->lastFrameNumUpdated != g.getFrameNum()) {
		sfons->lastFrameNumUpdated = g.getFrameNum();
		sfons->dirty			   = false;

		sg_image_data imgData		= {};
		imgData.subimage[0][0].ptr	= sfons->data;
		imgData.subimage[0][0].size = (size_t) (sfons->width * sfons->height);
		sg_update_image(sfons->tex, &imgData);
	}

	sfons->texture->bind();

	// interleave x,y,u,v - one buffer slot 0, stride 16
	std::vector<float> data((size_t) nverts * 4);
	for (int i = 0; i < nverts; i++) {
		data[i * 4 + 0] = verts[i * 2 + 0];
		data[i * 4 + 1] = verts[i * 2 + 1];
		data[i * 4 + 2] = tcoords[i * 2 + 0];
		data[i * 4 + 3] = tcoords[i * 2 + 1];
	}
	const size_t numBytes = data.size() * sizeof(float);

	if (sfons->appendBuf.id == SG_INVALID_ID && sg_isvalid()) {
		sg_buffer_desc desc = {};
		desc.size			= sokolFonsAppendBufSize;
		desc.usage			= SG_USAGE_STREAM;
		sfons->appendBuf	= sg_make_buffer(desc);
		SokolBufferTracker::track(sfons->appendBuf, sokolFonsAppendBufSize, false);
	}

	SokolShader *shader = sokolFons__pickShader(g);

	if (sfons->appendBuf.id == SG_INVALID_ID || shader == nullptr
		|| sg_query_buffer_will_overflow(sfons->appendBuf, numBytes)) {
		sokolFons__renderDrawPooled(sfons, verts, tcoords, nverts);
		sfons->texture->unbind();
		return;
	}

	const int byteOffset = sg_append_buffer(sfons->appendBuf, {data.data(), numBytes});
	SokolBufferTracker::touch(sfons->appendBuf);

	constexpr int stride = 4 * (int) sizeof(float);
	std::vector<SokolVertexAttr> attrs;
	if (int loc = shader->attrSlot("Position"); loc >= 0) {
		attrs.push_back({loc, SG_VERTEXFORMAT_FLOAT2, 0, false, 0, stride});
	}
	if (int loc = shader->attrSlot("TexCoord"); loc >= 0) {
		attrs.push_back({loc, SG_VERTEXFORMAT_FLOAT2, 0, false, 8, stride});
	}

	auto *api = static_cast<SokolAPI *>(&g.getAPI());
	shader->getPipeline(attrs, false, SG_PRIMITIVETYPE_TRIANGLES)->apply();

	sg_bindings bindings		  = {};
	bindings.vertex_buffers[0]	  = sfons->appendBuf;
	bindings.vertex_buffer_offsets[0] = byteOffset;
	bindings.fs.images[0]		  = api->getBoundTexture();
	bindings.fs.samplers[0]		  = api->getSampler();
	sg_apply_bindings(bindings);
	shader->applyUniforms();
	sg_draw(0, nverts, 1);

	sfons->texture->unbind();
}

static void sokolFons__renderDelete(void *userPtr) {
	sokolFONScontext *sfons = (sokolFONScontext *) userPtr;
	if (sfons->tex.id != SG_INVALID_ID && sg_isvalid()) {
		sg_destroy_image(sfons->tex);
	}
	if (sfons->appendBuf.id != SG_INVALID_ID) {
		if (sg_isvalid()) sg_destroy_buffer(sfons->appendBuf);
		SokolBufferTracker::untrack(sfons->appendBuf);
	}
	delete sfons;
}

FONS_DEF FONScontext *sokolFonsCreate(Graphics &g, int width, int height, int flags) {
	FONSparams params;

	sokolFONScontext *gl = new sokolFONScontext();
	gl->g				 = &g;
	memset(&params, 0, sizeof(params));

	params.width		= width;
	params.height		= height;
	params.flags		= (unsigned char) flags;
	params.renderCreate = sokolFons__renderCreate;
	params.renderResize = sokolFons__renderResize;
	params.renderUpdate = sokolFons__renderUpdate;
	params.renderDraw	= sokolFons__renderDraw;
	params.renderDelete = sokolFons__renderDelete;
	params.userPtr		= gl;
	gl->tex				= {SG_INVALID_ID};

	return fonsCreateInternal(&params);
}

FONS_DEF void sokolFonsDelete(FONScontext *ctx) {
	fonsDeleteInternal(ctx);
}
