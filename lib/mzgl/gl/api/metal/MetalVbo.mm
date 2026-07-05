#include "MetalVbo.h"
#include "MetalShader.h"
#include "MetalAPI.h"
#include "MetalContext.h"
#include "Graphics.h"
#include "Geometry.h"
#include "mzAssert.h"
#include "log.h"
#include <array>

// shared constants (buffer index base, setBytes limit) come from MetalContext.h

void mzglMetalBufferRelease(void *&buf) {
	if (buf == nullptr) return;
	// hand the buffer back to the pool (it becomes reusable after the current
	// frame's command buffer completes) instead of destroying it
	MetalBufferPool::instance().release(CFBridgingRelease(buf));
	buf = nullptr;
}

// lazily (re)acquire the MTLBuffer backing some CPU data. On a dirty update
// the old buffer is never written in place (an in-flight frame may still be
// reading it) - it goes back to the pool and a recycled, GPU-idle one is
// filled instead.
static id<MTLBuffer> ensureBuffer(void *&gpu, bool &gpuDirty, const std::vector<uint8_t> &cpuData) {
	if (cpuData.empty()) return nil;
	if (gpu != nullptr && !gpuDirty) return (__bridge id<MTLBuffer>) gpu;
	mzglMetalBufferRelease(gpu);
	id<MTLBuffer> buf = MetalBufferPool::instance().acquire(cpuData.size());
	memcpy(buf.contents, cpuData.data(), cpuData.size());
	gpu		 = (void *) CFBridgingRetain(buf);
	gpuDirty = false;
	return buf;
}

static MTLPrimitiveType primitiveTypeToMetal(Vbo::PrimitiveType mode) {
	switch (mode) {
		case Vbo::PrimitiveType::Triangles: return MTLPrimitiveTypeTriangle;
		case Vbo::PrimitiveType::TriangleStrip: return MTLPrimitiveTypeTriangleStrip;
		case Vbo::PrimitiveType::LineStrip: return MTLPrimitiveTypeLineStrip;
		case Vbo::PrimitiveType::Lines: return MTLPrimitiveTypeLine;
		default: {
			Log::e() << "ERROR!! invalid primitive type " << (int) mode;
			return MTLPrimitiveTypeTriangle;
		}
	}
}

MetalShader *MetalVbo::getShader(Graphics &g) const {
	// same auto-pick as SokolVbo::getShader
	if (g.currShader && !g.currShader->isDefaultShader) {
		return static_cast<MetalShader *>(g.currShader);
	}
	if (colorBuffer.present()) {
		if (texCoordBuffer.present()) {
			if (g.currShader == g.colorFontShader.get()) {
				return static_cast<MetalShader *>(g.colorFontShader.get());
			}
			return static_cast<MetalShader *>(g.colorTextureShader.get());
		}
		return static_cast<MetalShader *>(g.colorShader.get());
	}

	if (texCoordBuffer.present()) {
		if (g.currShader == g.fontShader.get()) {
			return static_cast<MetalShader *>(g.fontShader.get());
		}
		return static_cast<MetalShader *>(g.texShader.get());
	}
	return static_cast<MetalShader *>(g.nothingShader.get());
}

// bind the frame's texture/sampler (if any) to the fragment stage. Skips the
// registry lookup AND the encoder calls when the same texture/sampler is
// already bound (consecutive glyph/icon draws all share the atlas).
static void bindTexture(MetalAPI *api, id<MTLRenderCommandEncoder> encoder, bool hasTexCoords) {
	uint32_t texId = api->getBoundTexture();
	if (texId == 0) return;
	mzAssert(hasTexCoords, "Texture is bound but VBO has no tex coords - did you forget to unbind a texture?");
	auto &impl = api->impl();
	if (impl.curTexId != texId) {
		id<MTLTexture> tex = mzglMetal::lookupTexture(texId);
		if (tex == nil) return;
		[encoder setFragmentTexture:tex atIndex:0];
		impl.curTexId = texId;
	}
	const uint32_t samplerKey = api->getBoundSamplerKey();
	if (impl.curSamplerKey != samplerKey) {
		[encoder setFragmentSamplerState:impl.sampler(samplerKey) atIndex:0];
		impl.curSamplerKey = samplerKey;
	}
}

// vertex attrs per draw: at most Position/Color/TexCoord/InstanceID
using AttrArray = std::array<SokolVertexAttr, 4>;

void MetalVbo::draw_(Graphics &g, Vbo::PrimitiveType mode, size_t numInstances) {
	if (interleaved) {
		drawInterleaved(g, mode, numInstances);
		return;
	}

	auto *api  = static_cast<MetalAPI *>(&g.getAPI());
	auto &impl = api->impl();
	if (impl.encoder == nil) return;

	auto *shader = getShader(g);
	if (shader == nullptr) return;

	if (!positionBuffer.present()) {
		printf("position buffer not valid\n");
		return;
	}

	// Bind each vertex buffer to the shader attribute location resolved BY
	// NAME from the shader's reflection (same scheme as SokolVbo).
	AttrArray attrs;
	int numAttrs = 0;
	int slot	 = 0;

	auto bindAttr = [&](Buffer &buf, const char *attrName, bool perInstance = false) {
		if (!buf.present()) return;
		int location = shader->attrSlot(attrName);
		if (location < 0) return; // shader doesn't declare this attribute
		const int bufIdx = mzglMetalVertexBufferIndexBase + slot;
		if (buf.cpuData.size() <= mzglMetalSetBytesLimit) {
			[impl.encoder setVertexBytes:buf.cpuData.data() length:buf.cpuData.size() atIndex:bufIdx];
		} else {
			id<MTLBuffer> mtlBuf = ensureBuffer(buf.gpu, buf.gpuDirty, buf.cpuData);
			[impl.encoder setVertexBuffer:mtlBuf offset:0 atIndex:bufIdx];
		}
		attrs[numAttrs++] = {location, buf.getFormat(), slot, perInstance};
		slot++;
	};

	bindAttr(positionBuffer, "Position");
	bindAttr(colorBuffer, "Color");
	bindAttr(texCoordBuffer, "TexCoord");

	if (numInstances > 1) {
		if (!instanceIndexBuffer.present() || instanceIndexBuffer.size != (int) numInstances) {
			std::vector<float> counter;
			counter.reserve(numInstances);
			for (size_t i = 0; i < numInstances; i++) {
				counter.emplace_back((float) i);
			}
			instanceIndexBuffer.set(counter);
		}
		bindAttr(instanceIndexBuffer, "InstanceID", true);
	}

	bindTexture(api, impl.encoder, texCoordBuffer.present());

	if (!shader->applyPipelineAndUniforms(attrs.data(), numAttrs)) return;

	if (indexBuffer.present()) {
		id<MTLBuffer> idxBuf = ensureBuffer(indexBuffer.gpu, indexBuffer.gpuDirty, indexBuffer.cpuData);
		[impl.encoder drawIndexedPrimitives:primitiveTypeToMetal(mode)
								 indexCount:(NSUInteger) indexBuffer.size
								  indexType:MTLIndexTypeUInt32
								indexBuffer:idxBuf
						  indexBufferOffset:0
							  instanceCount:(NSUInteger) numInstances];
	} else {
		[impl.encoder drawPrimitives:primitiveTypeToMetal(mode)
						 vertexStart:0
						 vertexCount:(NSUInteger) positionBuffer.size
					   instanceCount:(NSUInteger) numInstances];
	}
}

// Pack all vertex attributes into a single interleaved buffer (pos[+col][+uv])
// - same layout as SokolVbo::setGeometry, the index buffer stays separate.
Vbo &MetalVbo::setGeometry(const Geometry &geom) {
	positionBuffer.deallocate();
	colorBuffer.deallocate();
	texCoordBuffer.deallocate();

	const int n = (int) geom.verts.size();
	if (n == 0) {
		il.deallocate();
		interleaved = false;
		indexBuffer.deallocate();
		return *this;
	}

	const bool hasCol = geom.cols.size() == (size_t) n;
	const bool hasUv  = geom.texCoords.size() == (size_t) n;

	const int floatsPerVert = 2 + (hasCol ? 4 : 0) + (hasUv ? 2 : 0);
	il.strideBytes			= floatsPerVert * (int) sizeof(float);
	int off					= 2;
	il.colOffset			= hasCol ? off * (int) sizeof(float) : -1;
	if (hasCol) off += 4;
	il.uvOffset = hasUv ? off * (int) sizeof(float) : -1;
	if (hasUv) off += 2;
	il.vertCount = n;

	// write straight into cpuData - per-frame drawers call this every frame,
	// so avoid a zero-filled temp + second copy (resize to the same size
	// doesn't touch existing bytes)
	il.cpuData.resize((size_t) floatsPerVert * n * sizeof(float));
	float *d = (float *) il.cpuData.data();
	for (int i = 0; i < n; i++) {
		*d++ = geom.verts[i].x;
		*d++ = geom.verts[i].y;
		if (hasCol) {
			*d++ = geom.cols[i].x;
			*d++ = geom.cols[i].y;
			*d++ = geom.cols[i].z;
			*d++ = geom.cols[i].w;
		}
		if (hasUv) {
			*d++ = geom.texCoords[i].x;
			*d++ = geom.texCoords[i].y;
		}
	}
	il.gpuDirty = true;
	interleaved = true;

	if (!geom.indices.empty()) {
		indexBuffer.set(geom.indices);
	} else {
		indexBuffer.deallocate();
	}
	return *this;
}

void MetalVbo::drawInterleaved(Graphics &g, Vbo::PrimitiveType mode, size_t numInstances) {
	auto *api  = static_cast<MetalAPI *>(&g.getAPI());
	auto &impl = api->impl();
	if (impl.encoder == nil) return;
	if (!il.present()) return;

	const bool hasCol = il.colOffset >= 0;
	const bool hasUv  = il.uvOffset >= 0;

	// mirror getShader()'s auto-pick, keyed on the interleaved presence flags
	MetalShader *shader = nullptr;
	if (g.currShader && !g.currShader->isDefaultShader) {
		shader = static_cast<MetalShader *>(g.currShader);
	} else if (hasCol && hasUv) {
		shader = static_cast<MetalShader *>(
			g.currShader == g.colorFontShader.get() ? g.colorFontShader.get() : g.colorTextureShader.get());
	} else if (hasCol) {
		shader = static_cast<MetalShader *>(g.colorShader.get());
	} else if (hasUv) {
		shader = static_cast<MetalShader *>(g.currShader == g.fontShader.get() ? g.fontShader.get()
																				 : g.texShader.get());
	} else {
		shader = static_cast<MetalShader *>(g.nothingShader.get());
	}
	if (shader == nullptr) return;

	AttrArray attrs;
	int numAttrs = 0;
	auto add	 = [&](const char *attrName, sg_vertex_format fmt, int offset) {
		int loc = shader->attrSlot(attrName);
		if (loc < 0) return;
		SokolVertexAttr a;
		a.location		  = loc;
		a.format		  = fmt;
		a.bufferSlot	  = 0;
		a.offset		  = offset;
		a.bufferStride	  = il.strideBytes;
		attrs[numAttrs++] = a;
	};
	add("Position", SG_VERTEXFORMAT_FLOAT2, 0);
	if (hasCol) add("Color", SG_VERTEXFORMAT_FLOAT4, il.colOffset);
	if (hasUv) add("TexCoord", SG_VERTEXFORMAT_FLOAT2, il.uvOffset);

	if (il.cpuData.size() <= mzglMetalSetBytesLimit) {
		[impl.encoder setVertexBytes:il.cpuData.data()
							  length:il.cpuData.size()
							 atIndex:mzglMetalVertexBufferIndexBase];
	} else {
		id<MTLBuffer> buf = ensureBuffer(il.gpu, il.gpuDirty, il.cpuData);
		[impl.encoder setVertexBuffer:buf offset:0 atIndex:mzglMetalVertexBufferIndexBase];
	}

	// per-instance attribute lives in its own buffer (slot 1)
	if (numInstances > 1) {
		if (!instanceIndexBuffer.present() || (size_t) instanceIndexBuffer.size != numInstances) {
			std::vector<float> counter;
			counter.reserve(numInstances);
			for (size_t i = 0; i < numInstances; i++)
				counter.emplace_back((float) i);
			instanceIndexBuffer.set(counter);
		}
		int loc = shader->attrSlot("InstanceID");
		if (loc >= 0) {
			id<MTLBuffer> instBuf =
				ensureBuffer(instanceIndexBuffer.gpu, instanceIndexBuffer.gpuDirty, instanceIndexBuffer.cpuData);
			[impl.encoder setVertexBuffer:instBuf offset:0 atIndex:mzglMetalVertexBufferIndexBase + 1];
			SokolVertexAttr a;
			a.location		  = loc;
			a.format		  = SG_VERTEXFORMAT_FLOAT;
			a.bufferSlot	  = 1;
			a.perInstance	  = true;
			attrs[numAttrs++] = a;
		}
	}

	bindTexture(api, impl.encoder, hasUv);

	if (!shader->applyPipelineAndUniforms(attrs.data(), numAttrs)) return;

	if (indexBuffer.present()) {
		id<MTLBuffer> idxBuf = ensureBuffer(indexBuffer.gpu, indexBuffer.gpuDirty, indexBuffer.cpuData);
		[impl.encoder drawIndexedPrimitives:primitiveTypeToMetal(mode)
								 indexCount:(NSUInteger) indexBuffer.size
								  indexType:MTLIndexTypeUInt32
								indexBuffer:idxBuf
						  indexBufferOffset:0
							  instanceCount:(NSUInteger) numInstances];
	} else {
		[impl.encoder drawPrimitives:primitiveTypeToMetal(mode)
						 vertexStart:0
						 vertexCount:(NSUInteger) il.vertCount
					   instanceCount:(NSUInteger) numInstances];
	}
}
