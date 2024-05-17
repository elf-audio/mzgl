
#pragma once
//#include "log.h"
#include "sokol_gfx.h"
#include "mzAssert.h"
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
};

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
	return sokolFons__renderCreate(userPtr, width, height);
}

static void sokolFons__renderUpdate(void *userPtr, int *rect, const unsigned char *data) {
	sokolFONScontext *gl = (sokolFONScontext *) userPtr;

	gl->data  = (unsigned char *) data;
	gl->dirty = true;
}

static void sokolFons__renderDraw(void *userPtr, const float *verts, const float *tcoords, int nverts) {
	sokolFONScontext *sfons = (sokolFONScontext *) userPtr;
	Graphics &g				= *sfons->g;

	if (sfons->dirty && sfons->lastFrameNumUpdated != g.getFrameNum()) {
		sfons->lastFrameNumUpdated = g.getFrameNum();
		sfons->dirty			   = false;

		sg_image_data imgData;
		memset(&imgData, 0, sizeof(imgData));
		imgData.subimage[0][0].ptr	= sfons->data;
		imgData.subimage[0][0].size = (size_t) (sfons->width * sfons->height);
		sg_update_image(sfons->tex, &imgData);
	}

	std::vector<glm::vec2> v;
	for (int i = 0; i < nverts; i++) {
		v.push_back(glm::vec2(verts[i * 2], verts[i * 2 + 1]));
	}
	std::vector<glm::vec2> t;
	for (int i = 0; i < nverts; i++) {
		t.push_back(glm::vec2(tcoords[i * 2], tcoords[i * 2 + 1]));
	}

	auto vbo = Vbo::create();
	vbo->setVertices(v);
	vbo->setTexCoords(t);

	sfons->texture->bind();
	//	g.fontShader->begin();
	//	g.fontShader->setMVP(g.getMVP());
	//	g.fontShader->setColor(g.getColor());

	vbo->draw(g);

	//	g.fontShader->end();
	sfons->texture->unbind();
}

static void sokolFons__renderDelete(void *userPtr) {
	sokolFONScontext *sfons = (sokolFONScontext *) userPtr;
	if (sfons->tex.id != SG_INVALID_ID) {
		sg_destroy_image(sfons->tex);
		sfons->tex.id = SG_INVALID_ID;
	}
}

FONS_DEF FONScontext *sokolFonsCreate(Graphics &g, int width, int height, int flags) {
	FONSparams params;
	sokolFONScontext *gl;

	gl = (sokolFONScontext *) malloc(sizeof(sokolFONScontext));
	memset(gl, 0, sizeof(sokolFONScontext));
	gl->g = &g;
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
