#include "D3D11Vbo.h"
#include "D3D11Shader.h"
#include "D3D11API.h"
#include "D3D11Internal.h"
#include "Graphics.h"
#include "Geometry.h"
#include "mzAssert.h"
#include "log.h"
#include <array>
#include <algorithm>

void mzglD3D11BufferRelease(void *&buf) {
	if (buf == nullptr) return;
	// the runtime defers the actual destruction until queued draws that still
	// read the buffer have completed, so releasing right away is safe
	static_cast<ID3D11Buffer *>(buf)->Release();
	buf = nullptr;
	mzglD3D11::state().liveBuffers--;
}

namespace {
	ID3D11Buffer *makeImmutable(const std::vector<uint8_t> &data, bool isIndex) {
		ID3D11Device *dev = mzglD3D11::device();
		if (dev == nullptr) return nullptr;
		D3D11_BUFFER_DESC d		  = {};
		d.ByteWidth				  = (UINT) data.size();
		d.Usage					  = D3D11_USAGE_IMMUTABLE;
		d.BindFlags				  = isIndex ? D3D11_BIND_INDEX_BUFFER : D3D11_BIND_VERTEX_BUFFER;
		D3D11_SUBRESOURCE_DATA sd = {};
		sd.pSysMem				  = data.data();
		ID3D11Buffer *buf		  = nullptr;
		if (!mzglD3D11Check(dev->CreateBuffer(&d, &sd, &buf), "CreateBuffer(immutable)")) return nullptr;
		mzglD3D11::state().liveBuffers++;
		return buf;
	}

	// bytes `d` would append to the stream ring if resolved now (0 when it
	// already has a usable GPU buffer); 16 extra for the append alignment
	uint32_t streamBytesNeeded(const D3D11Vbo::GpuData &d) {
		if (d.cpuData.empty()) return 0;
		if (d.gpu != nullptr && !d.gpuDirty && d.gpuGeneration == mzglD3D11::generation()) return 0;
		return (uint32_t) d.cpuData.size() + 16u;
	}

	// The buffer + byte offset to bind for this draw, or null. Streams the
	// data on its first draw (and every draw of a streaming Vbo); promotes
	// unchanged data to an IMMUTABLE buffer when it's drawn again in a later
	// frame. See the scheme in D3D11Vbo.h.
	ID3D11Buffer *resolve(D3D11Vbo::GpuData &d, bool streaming, bool isIndex, uint32_t frameNum, UINT &outOffset) {
		outOffset = 0;
		if (d.cpuData.empty()) return nullptr;
		const uint32_t gen = mzglD3D11::generation();
		if (d.gpu != nullptr && !d.gpuDirty && d.gpuGeneration == gen) {
			return static_cast<ID3D11Buffer *>(d.gpu);
		}
		mzglD3D11BufferRelease(d.gpu); // stale (data changed or device replaced)

		const bool drawnBefore = d.streamed && d.streamFrame != frameNum;
		const bool oversize	   = d.cpuData.size() > mzglD3D11::kStreamRingBytes;
		if ((!streaming && drawnBefore) || oversize) {
			d.gpu			= makeImmutable(d.cpuData, isIndex);
			d.gpuGeneration = gen;
			d.gpuDirty		= false;
			d.streamed		= false;
			return static_cast<ID3D11Buffer *>(d.gpu);
		}

		if (d.streamed && d.streamFrame == frameNum && d.streamEpoch == mzglD3D11::streamEpoch()) {
			// drawn more than once this frame: the ring still holds it
			outOffset = d.streamOffset;
			return mzglD3D11::streamBuffer();
		}
		uint32_t offset = 0;
		uint32_t epoch	= 0;
		if (!mzglD3D11::streamAppend(d.cpuData.data(), (uint32_t) d.cpuData.size(), offset, epoch)) {
			return nullptr;
		}
		d.streamed	   = true;
		d.streamOffset = offset;
		d.streamEpoch  = epoch;
		d.streamFrame  = frameNum;
		outOffset	   = offset;
		return mzglD3D11::streamBuffer();
	}

	D3D11_PRIMITIVE_TOPOLOGY toTopology(Vbo::PrimitiveType mode) {
		switch (mode) {
			case Vbo::PrimitiveType::Triangles: return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
			case Vbo::PrimitiveType::TriangleStrip: return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
			case Vbo::PrimitiveType::LineStrip: return D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP;
			case Vbo::PrimitiveType::Lines: return D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
			default: {
				Log::e() << "ERROR!! invalid primitive type " << (int) mode;
				return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
			}
		}
	}

	// bind the frame's texture/sampler (if any) to the pixel stage, skipping
	// the registry lookup and the context calls when nothing changed
	// (consecutive glyph/icon draws all share the atlas)
} // namespace

void mzglD3D11::applyTextureBinding(D3D11API &apiRef, bool hasTexCoords) {
	auto *api	   = &apiRef;
	auto &s		   = mzglD3D11::state();
	auto &m		   = s.memo;
	auto *dc	   = s.context;
	uint32_t texId = api->getBoundTexture();
	if (texId == 0) {
		if (m.srv != nullptr) {
			ID3D11ShaderResourceView *nullSrv = nullptr;
			dc->PSSetShaderResources(0, 1, &nullSrv);
			m.srv	= nullptr;
			m.texId = 0;
		}
		return;
	}
	mzAssert(hasTexCoords, "Texture is bound but VBO has no tex coords - did you forget to unbind a texture?");
	if (m.texId != texId) {
		ID3D11ShaderResourceView *srv = mzglD3D11::lookupTexture(texId);
		if (m.srv != srv) {
			dc->PSSetShaderResources(0, 1, &srv);
			m.srv = srv;
		}
		m.texId = texId;
	}
	ID3D11SamplerState *smp = mzglD3D11::sampler(api->getBoundSamplerKey());
	if (m.sampler != smp) {
		dc->PSSetSamplers(0, 1, &smp);
		m.sampler = smp;
	}
}

namespace {
	void issueDraw(ID3D11DeviceContext *dc, bool indexed, int count, size_t numInstances) {
		if (indexed) {
			if (numInstances > 1) {
				dc->DrawIndexedInstanced((UINT) count, (UINT) numInstances, 0, 0, 0);
			} else {
				dc->DrawIndexed((UINT) count, 0, 0);
			}
		} else {
			if (numInstances > 1) {
				dc->DrawInstanced((UINT) count, (UINT) numInstances, 0, 0);
			} else {
				dc->Draw((UINT) count, 0);
			}
		}
	}

	// vertex attrs per draw: at most Position/Color/TexCoord/InstanceID
	using AttrArray = std::array<SokolVertexAttr, 4>;

	// MZGL_D3D11_TRACE: screen-space bounds of a draw's positions
	void traceBounds(Graphics &g, const uint8_t *data, int strideBytes, int comps, int count) {
		if (!mzglD3D11::state().traceThisFrame || count <= 0) return;
		const glm::mat4 mvp = g.getMVP();
		float x0 = 1e9f, y0 = 1e9f, x1 = -1e9f, y1 = -1e9f;
		for (int i = 0; i < count; i++) {
			const float *p = (const float *) (data + (size_t) i * strideBytes);
			glm::vec4 v(p[0], p[1], comps > 2 ? p[2] : 0.f, comps > 3 ? p[3] : 1.f);
			glm::vec4 c = mvp * v;
			if (c.w == 0.f) continue;
			const float sx = (c.x / c.w + 1.f) * 0.5f * g.width;
			const float sy = (1.f - c.y / c.w) * 0.5f * g.height;
			x0			   = std::min(x0, sx);
			y0			   = std::min(y0, sy);
			x1			   = std::max(x1, sx);
			y1			   = std::max(y1, sy);
		}
		Log::d() << "[D3D11 trace]   verts=" << count << " px bbox=(" << x0 << "," << y0 << ")-(" << x1 << ","
				 << y1 << ")";
	}
} // namespace

D3D11Shader *D3D11Vbo::getShader(Graphics &g) const {
	// same auto-pick as SokolVbo::getShader
	if (g.currShader && !g.currShader->isDefaultShader) {
		return static_cast<D3D11Shader *>(g.currShader);
	}
	if (colorBuffer.present()) {
		if (texCoordBuffer.present()) {
			if (g.currShader == g.colorFontShader.get()) {
				return static_cast<D3D11Shader *>(g.colorFontShader.get());
			}
			return static_cast<D3D11Shader *>(g.colorTextureShader.get());
		}
		return static_cast<D3D11Shader *>(g.colorShader.get());
	}

	if (texCoordBuffer.present()) {
		if (g.currShader == g.fontShader.get()) {
			return static_cast<D3D11Shader *>(g.fontShader.get());
		}
		return static_cast<D3D11Shader *>(g.texShader.get());
	}
	return static_cast<D3D11Shader *>(g.nothingShader.get());
}

void D3D11Vbo::draw_(Graphics &g, Vbo::PrimitiveType mode, size_t numInstances) {
	if (interleaved) {
		drawInterleaved(g, mode, numInstances);
		return;
	}

	auto *api = static_cast<D3D11API *>(&g.getAPI());
	if (!api->inFrame()) return;

	auto *shader = getShader(g);
	if (shader == nullptr) return;

	if (!positionBuffer.present()) {
		printf("position buffer not valid\n");
		return;
	}

	auto *dc				= mzglD3D11::context();
	const uint32_t frameNum = g.getFrameNum();

	// Bind each vertex buffer to the shader attribute location resolved BY
	// NAME from the shader's reflection (same scheme as SokolVbo).
	AttrArray attrs;
	int numAttrs = 0;
	int slot	 = 0;

	auto bindAttr = [&](Buffer &buf, const char *attrName, bool perInstance = false) {
		if (!buf.present()) return;
		int location = shader->attrSlot(attrName);
		if (location < 0) return; // shader doesn't declare this attribute
		UINT offset		  = 0;
		ID3D11Buffer *gpu = resolve(buf, streaming, false, frameNum, offset);
		if (gpu == nullptr) return;
		const UINT stride = (UINT) buf.unitSize;
		dc->IASetVertexBuffers((UINT) slot, 1, &gpu, &stride, &offset);
		attrs[numAttrs++] = {location, buf.getFormat(), slot, perInstance};
		slot++;
	};

	if (numInstances > 1 && (!instanceIndexBuffer.present() || instanceIndexBuffer.size != (int) numInstances)) {
		std::vector<float> counter;
		counter.reserve(numInstances);
		for (size_t i = 0; i < numInstances; i++) {
			counter.emplace_back((float) i);
		}
		instanceIndexBuffer.set(counter);
	}
	// everything this draw streams must fit without a wrap (see streamReserve)
	mzglD3D11::streamReserve(streamBytesNeeded(positionBuffer) + streamBytesNeeded(colorBuffer)
							 + streamBytesNeeded(texCoordBuffer) + streamBytesNeeded(indexBuffer)
							 + (numInstances > 1 ? streamBytesNeeded(instanceIndexBuffer) : 0u));

	bindAttr(positionBuffer, "Position");
	bindAttr(colorBuffer, "Color");
	bindAttr(texCoordBuffer, "TexCoord");
	if (numInstances > 1) bindAttr(instanceIndexBuffer, "InstanceID", true);

	mzglD3D11::applyTextureBinding(*api, texCoordBuffer.present());
	mzglD3D11::setTopology(toTopology(mode));

	if (!shader->applyPipelineAndUniforms(attrs.data(), numAttrs)) return;
	traceBounds(g,
				positionBuffer.cpuData.data(),
				positionBuffer.unitSize,
				positionBuffer.unitSize / 4,
				positionBuffer.size);

	if (indexBuffer.present()) {
		UINT offset		  = 0;
		ID3D11Buffer *idx = resolve(indexBuffer, streaming, true, frameNum, offset);
		if (idx == nullptr) return;
		dc->IASetIndexBuffer(idx, DXGI_FORMAT_R32_UINT, offset);
		issueDraw(dc, true, indexBuffer.size, numInstances);
	} else {
		issueDraw(dc, false, positionBuffer.size, numInstances);
	}
}

// Pack all vertex attributes into a single interleaved buffer (pos[+col][+uv])
// - same layout as SokolVbo::setGeometry, the index buffer stays separate.
Vbo &D3D11Vbo::setGeometry(const Geometry &geom) {
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
	il.markChanged();
	interleaved = true;

	if (!geom.indices.empty()) {
		indexBuffer.set(geom.indices);
	} else {
		indexBuffer.deallocate();
	}
	return *this;
}

void D3D11Vbo::drawInterleaved(Graphics &g, Vbo::PrimitiveType mode, size_t numInstances) {
	auto *api = static_cast<D3D11API *>(&g.getAPI());
	if (!api->inFrame()) return;
	if (!il.present()) return;

	const bool hasCol = il.colOffset >= 0;
	const bool hasUv  = il.uvOffset >= 0;

	// mirror getShader()'s auto-pick, keyed on the interleaved presence flags
	D3D11Shader *shader = nullptr;
	if (g.currShader && !g.currShader->isDefaultShader) {
		shader = static_cast<D3D11Shader *>(g.currShader);
	} else if (hasCol && hasUv) {
		shader = static_cast<D3D11Shader *>(g.currShader == g.colorFontShader.get() ? g.colorFontShader.get()
																					: g.colorTextureShader.get());
	} else if (hasCol) {
		shader = static_cast<D3D11Shader *>(g.colorShader.get());
	} else if (hasUv) {
		shader = static_cast<D3D11Shader *>(g.currShader == g.fontShader.get() ? g.fontShader.get()
																			   : g.texShader.get());
	} else {
		shader = static_cast<D3D11Shader *>(g.nothingShader.get());
	}
	if (shader == nullptr) return;

	auto *dc				= mzglD3D11::context();
	const uint32_t frameNum = g.getFrameNum();

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

	if (numInstances > 1
		&& (!instanceIndexBuffer.present() || (size_t) instanceIndexBuffer.size != numInstances)) {
		std::vector<float> counter;
		counter.reserve(numInstances);
		for (size_t i = 0; i < numInstances; i++)
			counter.emplace_back((float) i);
		instanceIndexBuffer.set(counter);
	}
	// everything this draw streams must fit without a wrap (see streamReserve)
	mzglD3D11::streamReserve(streamBytesNeeded(il) + streamBytesNeeded(indexBuffer)
							 + (numInstances > 1 ? streamBytesNeeded(instanceIndexBuffer) : 0u));

	{
		UINT offset		  = 0;
		ID3D11Buffer *gpu = resolve(il, streaming, false, frameNum, offset);
		if (gpu == nullptr) return;
		const UINT stride = (UINT) il.strideBytes;
		dc->IASetVertexBuffers(0, 1, &gpu, &stride, &offset);
	}

	// per-instance attribute lives in its own buffer (slot 1)
	if (numInstances > 1) {
		int loc = shader->attrSlot("InstanceID");
		if (loc >= 0) {
			UINT offset		   = 0;
			ID3D11Buffer *inst = resolve(instanceIndexBuffer, false, false, frameNum, offset);
			if (inst != nullptr) {
				const UINT stride = sizeof(float);
				dc->IASetVertexBuffers(1, 1, &inst, &stride, &offset);
				SokolVertexAttr a;
				a.location		  = loc;
				a.format		  = SG_VERTEXFORMAT_FLOAT;
				a.bufferSlot	  = 1;
				a.perInstance	  = true;
				attrs[numAttrs++] = a;
			}
		}
	}

	mzglD3D11::applyTextureBinding(*api, hasUv);
	mzglD3D11::setTopology(toTopology(mode));

	if (!shader->applyPipelineAndUniforms(attrs.data(), numAttrs)) return;
	traceBounds(g, il.cpuData.data(), il.strideBytes, 2, il.vertCount);

	if (indexBuffer.present()) {
		UINT offset		  = 0;
		ID3D11Buffer *idx = resolve(indexBuffer, streaming, true, frameNum, offset);
		if (idx == nullptr) return;
		dc->IASetIndexBuffer(idx, DXGI_FORMAT_R32_UINT, offset);
		issueDraw(dc, true, indexBuffer.size, numInstances);
	} else {
		issueDraw(dc, false, il.vertCount, numInstances);
	}
}
