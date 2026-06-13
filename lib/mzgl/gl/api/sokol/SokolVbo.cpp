//
// Created by Marek Bereza on 10/05/2024.
//

#include "mzgl_platform.h"
CLANG_IGNORE_WARNINGS_BEGIN("-Wshorten-64-to-32")
#include "SokolVbo.h"
CLANG_IGNORE_WARNINGS_END
#include "Graphics.h"
#include "SokolPipeline.h"
#include "SokolAPI.h"
#include "mzAssert.h"

void SokolVbo::Buffer::deallocate() {
	if (valid()) {
		if (pool) {
			pool->release({buffer, pooledCapacity}, isIndex);
		} else if (sg_isvalid()) {
			// Skip when sokol is already shut down (layer/Vbo teardown can run
			// after sg_shutdown) - the buffer is already freed, and calling
			// sg_destroy_buffer on a dead context dereferences freed state.
			sg_destroy_buffer(buffer);
		}
		buffer.id	   = 0;
		pooledCapacity = 0;
	}
}
SokolShader *SokolVbo::getShader(Graphics &g) const {
	if (g.currShader && !g.currShader->isDefaultShader) {
		return dynamic_cast<SokolShader *>(g.currShader);
	}
	if (colorBuffer.valid()) {
		if (texCoordBuffer.valid()) {
			// colorFontShader and colorTextureShader share the same vertex layout
			// (pos+texcoord+color) so the auto-pick can't tell them apart. They
			// differ in the fragment stage: colorFont treats the texture as an R8
			// alpha mask (a *= tex.r), colorTexture multiplies rgba. Honour an
			// explicitly-bound colorFontShader, else assume a real texture.
			if (g.currShader == g.colorFontShader.get()) {
				return dynamic_cast<SokolShader *>(g.colorFontShader.get());
			}
			return dynamic_cast<SokolShader *>(g.colorTextureShader.get());
		}
		return dynamic_cast<SokolShader *>(g.colorShader.get());
	}

	if (texCoordBuffer.valid()) {
		if (g.currShader == g.fontShader.get()) {
			return dynamic_cast<SokolShader *>(g.fontShader.get());
		}
		return dynamic_cast<SokolShader *>(g.texShader.get());
	}
	return dynamic_cast<SokolShader *>(g.nothingShader.get());
}

static sg_primitive_type primitiveTypeToSokolMode(Vbo::PrimitiveType mode) {
	switch (mode) {
		case Vbo::PrimitiveType::Triangles: return SG_PRIMITIVETYPE_TRIANGLES;
		case Vbo::PrimitiveType::TriangleStrip: return SG_PRIMITIVETYPE_TRIANGLE_STRIP;
		case Vbo::PrimitiveType::LineStrip: return SG_PRIMITIVETYPE_LINE_STRIP;
		case Vbo::PrimitiveType::Lines: return SG_PRIMITIVETYPE_LINES;

		default: {
			Log::e() << "ERROR!! invalid primitive type " << (int) mode;
			return SG_PRIMITIVETYPE_TRIANGLES;
		}
	}
}

void SokolVbo::draw_(Graphics &g, Vbo::PrimitiveType mode, size_t numInstances) {
	auto shader = getShader(g);

	if (!positionBuffer.valid()) {
		printf("position buffer not valid\n");
		return;
	}

	sg_bindings bindings = {};
	std::vector<SokolVertexAttr> attrs;
	int slot = 0;

	// Bind each vertex buffer to the shader attribute location resolved BY NAME
	// from the shader's reflection (attrSlot), rather than assuming buffer slot i
	// feeds attribute location i. This makes the order attributes are declared in
	// the .glsl irrelevant: shaders that list Color before TexCoord (fx sliders,
	// piano roll notes) and ones that list TexCoord before Color (font atlas) both
	// bind correctly. Relying on order silently swaps attributes when they differ.
	auto bindAttr = [&](Buffer &buf, const char *attrName, bool perInstance = false) {
		if (!buf.valid()) return;
		int location = shader->attrSlot(attrName);
		if (location < 0) return; // shader doesn't declare this attribute
		bindings.vertex_buffers[slot] = buf.buffer;
		attrs.push_back({location, buf.getFormat(), slot, perInstance});
		slot++;
	};

	bindAttr(positionBuffer, "Position");
	bindAttr(colorBuffer, "Color");
	bindAttr(texCoordBuffer, "TexCoord");
	bindAttr(normalBuffer, "Normal");

	if (indexBuffer.valid()) {
		bindings.index_buffer = indexBuffer.buffer;
	}

	if (numInstances > 1) {
		if (!instanceIndexBuffer.valid() || instanceIndexBuffer.size != numInstances) {
			std::vector<float> counter;
			for (int i = 0; i < numInstances; i++) {
				counter.emplace_back(i);
			}
			instanceIndexBuffer.set(counter);
		}
		bindAttr(instanceIndexBuffer, "InstanceID", true);
	}

	auto *api = static_cast<SokolAPI *>(&g.getAPI());

	auto tex = api->getBoundTexture();
	if (tex.id != 0) {
		mzAssert(texCoordBuffer.valid(),
				 "Texture is bound but VBO has no tex coords - did you forget to unbind a texture?");
		bindings.fs.images[0]	= tex;
		auto sampler			= api->getSampler();
		bindings.fs.samplers[0] = sampler;
	}
	shader->getPipeline(attrs, indexBuffer.valid(), primitiveTypeToSokolMode(mode))->apply();

	sg_apply_bindings(bindings);

	int numVerts = positionBuffer.size;
	if (indexBuffer.valid()) numVerts = indexBuffer.size;
	shader->applyUniforms();
	sg_draw(0, numVerts, static_cast<int>(numInstances));
}
