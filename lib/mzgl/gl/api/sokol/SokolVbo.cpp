//
// Created by Marek Bereza on 10/05/2024.
//
#include "SokolVbo.h"
#include "Graphics.h"
#include "SokolPipeline.h"
#include "SokolAPI.h"

void SokolVbo::Buffer::deallocate() {
	if (valid()) {
		sg_destroy_buffer(buffer);
		buffer.id = 0;
	}
}
SokolShader *SokolVbo::getShader(Graphics &g) const {
	if (g.currShader && !g.currShader->isDefaultShader) {
		return dynamic_cast<SokolShader *>(g.currShader);
	}
	if (colorBuffer.valid()) {
		if (texCoordBuffer.valid()) {
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

	std::vector<sg_vertex_format> attrs = {positionBuffer.getFormat()};

	if (!positionBuffer.valid()) {
		printf("position buffer not valid\n");
		return;
	}
	// vbo chooses the shader
	sg_bindings bindings = {.vertex_buffers[0] = positionBuffer.buffer};
	int nextPos			 = 1;

	if (colorBuffer.valid()) {
		bindings.vertex_buffers[nextPos] = colorBuffer.buffer;
		attrs.push_back(colorBuffer.getFormat());
		nextPos++;
	}

	if (texCoordBuffer.valid()) {
		bindings.vertex_buffers[nextPos] = texCoordBuffer.buffer;
		attrs.push_back(texCoordBuffer.getFormat());
		nextPos++;
	}

	if (normalBuffer.valid()) {
		bindings.vertex_buffers[nextPos] = normalBuffer.buffer;
		attrs.push_back(normalBuffer.getFormat());
		nextPos++;
	}

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
		attrs.push_back(instanceIndexBuffer.getFormat());
		bindings.vertex_buffers[nextPos] = instanceIndexBuffer.buffer;
		nextPos++;
	}
	auto *api = static_cast<SokolAPI *>(&g.getAPI());

	auto tex = api->getBoundTexture();

	if (tex.id != 0) {
		bindings.fs.images[0]	= tex;
		auto sampler			= api->getSampler();
		bindings.fs.samplers[0] = sampler;
	}
	shader->getPipeline(attrs, indexBuffer.valid(), primitiveTypeToSokolMode(mode), numInstances > 1)->apply();

	sg_apply_bindings(bindings);

	int numVerts = positionBuffer.size;
	if (indexBuffer.valid()) numVerts = indexBuffer.size;
	shader->applyUniforms();
	sg_draw(0, numVerts, numInstances);
}
