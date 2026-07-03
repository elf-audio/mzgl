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
#include "Geometry.h"
#include <chrono>
#include <mutex>
#include <unordered_set>

// Global registry of lazily-backed buffers, swept from a sokol commit listener:
// GPU buffers unused for kLazyMaxAgeMs get destroyed (their CPU copy is kept, so
// they're recreated transparently on the next draw). This bounds sokol
// buffer-pool slot usage to roughly "what's been on screen recently".
namespace {
	constexpr int64_t kLazyMaxAgeMs		 = 4000;
	constexpr int64_t kLazySweepEveryMs	 = 1000;

	struct LazySweep {
		std::mutex mut;
		std::unordered_set<SokolLazyBuffer *> buffers;
		bool listenerAttached = false;
		sg_commit_listener listener {};
		int64_t lastSweepMs = 0;

		static LazySweep &instance() {
			// Deliberately leaked: Vbos owned by globals/statics unregister from
			// their exit-time destructors, which must not touch a destroyed mutex.
			static LazySweep *s = new LazySweep();
			return *s;
		}

		static int64_t now() {
			return std::chrono::duration_cast<std::chrono::milliseconds>(
					   std::chrono::steady_clock::now().time_since_epoch())
				.count();
		}

		void add(SokolLazyBuffer *b) {
			std::lock_guard<std::mutex> l(mut);
			buffers.insert(b);
			if (!listenerAttached && sg_isvalid()) {
				listener.func	   = [](void *ud) { static_cast<LazySweep *>(ud)->sweep(); };
				listener.user_data = this;
				sg_add_commit_listener(listener);
				listenerAttached = true;
			}
		}

		void remove(SokolLazyBuffer *b) {
			std::lock_guard<std::mutex> l(mut);
			buffers.erase(b);
		}

		void sweep() {
			if (!SokolLazyBuffer::lazyModeEnabled()) return;
			auto t = now();
			if (t - lastSweepMs < kLazySweepEveryMs) return;
			lastSweepMs = t;
			std::lock_guard<std::mutex> l(mut);
			for (auto *b: buffers)
				b->evictIfUnusedSince(t - kLazyMaxAgeMs);
		}
	};
} // namespace

SokolLazyBuffer::~SokolLazyBuffer() {
	unregisterLazy();
}

void SokolLazyBuffer::registerLazy() {
	if (lazyRegistered) return;
	lazyRegistered = true;
	LazySweep::instance().add(this);
}

void SokolLazyBuffer::unregisterLazy() {
	if (!lazyRegistered) return;
	lazyRegistered = false;
	LazySweep::instance().remove(this);
}

int64_t SokolLazyBuffer::nowMs() {
	return LazySweep::now();
}

void SokolVbo::Buffer::deallocate() {
	if (valid()) {
		if (pool) {
			pool->release({buffer, pooledCapacity}, isIndex);
		} else if (sg_isvalid()) {
			// Skip when sokol is already shut down (layer/Vbo teardown can run
			// after sg_shutdown) - the buffer is already freed, and calling
			// sg_destroy_buffer on a dead context dereferences freed state.
			sg_destroy_buffer(buffer);
			SokolBufferTracker::untrack(buffer);
		}
		buffer.id	   = 0;
		pooledCapacity = 0;
	}
	cpuData.clear();
}
SokolShader *SokolVbo::getShader(Graphics &g) const {
	if (g.currShader && !g.currShader->isDefaultShader) {
		return dynamic_cast<SokolShader *>(g.currShader);
	}
	if (colorBuffer.present()) {
		if (texCoordBuffer.present()) {
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

	if (texCoordBuffer.present()) {
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
	if (interleaved) {
		drawInterleaved(g, mode, numInstances);
		return;
	}

	auto shader = getShader(g);

	if (!positionBuffer.present()) {
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
		buf.ensure(); // lazily (re)create the GPU buffer if it was evicted or never made
		if (!buf.valid()) return;
		int location = shader->attrSlot(attrName);
		if (location < 0) return; // shader doesn't declare this attribute
		bindings.vertex_buffers[slot] = buf.buffer;
		SokolBufferTracker::touch(buf.buffer);
		attrs.push_back({location, buf.getFormat(), slot, perInstance});
		slot++;
	};

	bindAttr(positionBuffer, "Position");
	bindAttr(colorBuffer, "Color");
	bindAttr(texCoordBuffer, "TexCoord");

	indexBuffer.ensure();
	if (indexBuffer.valid()) {
		bindings.index_buffer = indexBuffer.buffer;
		SokolBufferTracker::touch(indexBuffer.buffer);
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
		mzAssert(texCoordBuffer.present(),
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

// Pack all vertex attributes into a single interleaved buffer (pos[+col][+uv]).
// The index buffer stays separate. This trades 2-3 sokol buffers per VBO for 1
// (+1 index), cutting the live-buffer count that fills sokol's global pool.
Vbo &SokolVbo::setGeometry(const Geometry &geom) {
	// setGeometry owns all vertex attributes. Free any per-attribute buffers a
	// previous setVertices/setColors/etc may have created so they don't linger
	// alongside the interleaved buffer (draw_ would ignore them and they'd leak).
	positionBuffer.deallocate();
	colorBuffer.deallocate();
	texCoordBuffer.deallocate();

	// note: il is NOT deallocated here - setPooled() reuses it in place for
	// per-frame pooled VBOs; setLazy() deallocates before replacing.
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

	std::vector<float> packed((size_t) floatsPerVert * n);
	for (int i = 0; i < n; i++) {
		float *d = packed.data() + (size_t) i * floatsPerVert;
		*d++	 = geom.verts[i].x;
		*d++	 = geom.verts[i].y;
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

	if (pool) {
		// pooled (per-frame) VBOs stream into a recycled buffer instead of
		// creating/destroying an immutable one every set
		il.setPooled(packed.data(), packed.size() * sizeof(float), pool);
	} else {
		il.setLazy(packed.data(), packed.size() * sizeof(float));
	}
	interleaved = true;

	if (!geom.indices.empty()) {
		indexBuffer.set(geom.indices, pool);
	} else {
		indexBuffer.deallocate();
	}
	return *this;
}

void SokolVbo::drawInterleaved(Graphics &g, Vbo::PrimitiveType mode, size_t numInstances) {
	il.ensure();
	if (!il.valid()) return;

	const bool hasCol = il.colOffset >= 0;
	const bool hasUv  = il.uvOffset >= 0;

	// Mirror getShader()'s auto-pick, but keyed on the interleaved presence flags
	// rather than the (unused) per-attribute buffers.
	SokolShader *shader = nullptr;
	if (g.currShader && !g.currShader->isDefaultShader) {
		shader = dynamic_cast<SokolShader *>(g.currShader);
	} else if (hasCol && hasUv) {
		shader = dynamic_cast<SokolShader *>(
			g.currShader == g.colorFontShader.get() ? g.colorFontShader.get() : g.colorTextureShader.get());
	} else if (hasCol) {
		shader = dynamic_cast<SokolShader *>(g.colorShader.get());
	} else if (hasUv) {
		shader = dynamic_cast<SokolShader *>(g.currShader == g.fontShader.get() ? g.fontShader.get()
																				 : g.texShader.get());
	} else {
		shader = dynamic_cast<SokolShader *>(g.nothingShader.get());
	}

	sg_bindings bindings	   = {};
	bindings.vertex_buffers[0] = il.buffer;
	SokolBufferTracker::touch(il.buffer);

	std::vector<SokolVertexAttr> attrs;
	auto add = [&](const char *name, sg_vertex_format fmt, int offset) {
		int loc = shader->attrSlot(name);
		if (loc < 0) return;
		SokolVertexAttr a;
		a.location	   = loc;
		a.format	   = fmt;
		a.bufferSlot   = 0;
		a.offset	   = offset;
		a.bufferStride = il.strideBytes;
		attrs.push_back(a);
	};
	add("Position", SG_VERTEXFORMAT_FLOAT2, 0);
	if (hasCol) add("Color", SG_VERTEXFORMAT_FLOAT4, il.colOffset);
	if (hasUv) add("TexCoord", SG_VERTEXFORMAT_FLOAT2, il.uvOffset);

	// Per-instance attribute lives in its own buffer (slot 1), stepped per instance.
	if (numInstances > 1) {
		if (!instanceIndexBuffer.present() || (size_t) instanceIndexBuffer.size != numInstances) {
			std::vector<float> counter;
			counter.reserve(numInstances);
			for (size_t i = 0; i < numInstances; i++)
				counter.emplace_back((float) i);
			instanceIndexBuffer.set(counter);
		}
		instanceIndexBuffer.ensure();
		int loc = shader->attrSlot("InstanceID");
		if (loc >= 0) {
			bindings.vertex_buffers[1] = instanceIndexBuffer.buffer;
			SokolBufferTracker::touch(instanceIndexBuffer.buffer);
			SokolVertexAttr a;
			a.location	  = loc;
			a.format	  = SG_VERTEXFORMAT_FLOAT;
			a.bufferSlot  = 1;
			a.perInstance = true;
			attrs.push_back(a);
		}
	}

	indexBuffer.ensure();
	if (indexBuffer.valid()) {
		bindings.index_buffer = indexBuffer.buffer;
		SokolBufferTracker::touch(indexBuffer.buffer);
	}

	auto *api = static_cast<SokolAPI *>(&g.getAPI());
	auto tex  = api->getBoundTexture();
	if (tex.id != 0) {
		mzAssert(hasUv, "Texture is bound but interleaved VBO has no tex coords");
		bindings.fs.images[0]	= tex;
		bindings.fs.samplers[0] = api->getSampler();
	}

	shader->getPipeline(attrs, indexBuffer.valid(), primitiveTypeToSokolMode(mode))->apply();
	sg_apply_bindings(bindings);
	int numVerts = indexBuffer.valid() ? indexBuffer.size : il.vertCount;
	shader->applyUniforms();
	sg_draw(0, numVerts, static_cast<int>(numInstances));
}
