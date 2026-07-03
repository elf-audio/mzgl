#include "Graphics.h"
#include "SokolPipeline.h"
#include "SokolAPI.h"
#include "mzgl_platform.h"
CLANG_IGNORE_WARNINGS_BEGIN("-Wshorten-64-to-32")
#include "SokolShader.h"
CLANG_IGNORE_WARNINGS_END
#include <mutex>

// Process-global cache of compiled sg_shaders and their pipelines, shared by
// every Graphics/plugin instance in the process. The compiled shader for a
// given ShaderDef is byte-identical in every instance (per-draw uniform state
// lives CPU-side in each SokolShader and is applied with sg_apply_uniforms),
// so N instances only need one sg_shader + one pipeline set between them.
// Without this, ~16 shaders per instance exhaust the 128-slot shader pool at
// 8 plugin instances (SHADER_POOL_EXHAUSTED).
//
// Keyed by the ShaderDef's descFunc pointer - that uniquely identifies the
// compiled-in shader code, independent of registry names.
namespace {
	struct SharedShader {
		sg_shader shd = {SG_INVALID_ID};
		int refs	  = 0;
		std::map<size_t, PipelineRef> pipelines;
	};

	struct ShaderCache {
		std::mutex mut;
		std::map<ShaderDescFn, SharedShader> entries;

		static ShaderCache &instance() {
			// deliberately leaked: shader destructors run at exit time and must
			// not touch a destroyed mutex (see SokolBufferTracker for the same
			// pattern)
			static ShaderCache *c = new ShaderCache();
			return *c;
		}
	};
} // namespace

SokolShader::SokolShader(Graphics &g, const std::string &shaderName)
	: Shader(g) {
	name	  = shaderName;
	auto *api = static_cast<SokolAPI *>(&g.getAPI());
	def		  = api->getShaderDef(shaderName);
	{
		auto &cache = ShaderCache::instance();
		std::lock_guard<std::mutex> l(cache.mut);
		// std::map nodes are address-stable, so the entry pointer stays valid
		// for this SokolShader's lifetime (we hold a ref until our destructor)
		auto &e		= cache.entries[def->descFunc];
		sharedEntry = &e;
		// also recreate if the cached handle went stale (sg_shutdown + re-setup
		// can invalidate handles that entries still hold refs to)
		if (e.refs == 0 || sg_query_shader_state(e.shd) != SG_RESOURCESTATE_VALID) {
			e.shd = sg_make_shader(def->descFunc(sg_query_backend()));
			if (e.shd.id == SG_INVALID_ID) {
				Log::e() << "sg_make_shader failed for '" << shaderName << "' - shader pool exhausted?";
			}
		}
		e.refs++;
		shd = e.shd;
	}
	vertUniforms.resize(def->uniformblockSizeFunc(SG_SHADERSTAGE_VS, "vertParams"), 0);
	fragUniforms.resize(def->uniformblockSizeFunc(SG_SHADERSTAGE_FS, "fragParams"), 0);

	vertParamRange = sg_range {vertUniforms.data(), vertUniforms.size()};
	fragParamRange = sg_range {fragUniforms.data(), fragUniforms.size()};

	colorUniformOffset = uniformLocation("color");
	mvpUniformOffset   = uniformLocation("mvp");
	mzAssert(mvpUniformOffset != nullptr, "mvp uniform not found in shader");
}

SokolShader::~SokolShader() {
	auto &cache = ShaderCache::instance();
	std::lock_guard<std::mutex> l(cache.mut);
	auto *e = static_cast<SharedShader *>(sharedEntry);
	if (e == nullptr) return;
	if (--e->refs <= 0) {
		// destroy the pipelines first - they reference the shader.
		// sg_isvalid() guards live in ~Pipeline and here: the layer tree can
		// outlive sg_shutdown() on close.
		e->pipelines.clear();
		if (sg_isvalid() && e->shd.id != SG_INVALID_ID) {
			sg_destroy_shader(e->shd);
		}
		cache.entries.erase(def->descFunc);
	}
}

void SokolShader::begin() {
	g.currShader = this;
}
void SokolShader::end() {
	g.currShader = nullptr;
}

int SokolShader::attrSlot(const std::string &attrName) const {
	return def->attrSlotFunc(attrName.c_str());
}

PipelineRef SokolShader::getPipeline(const std::vector<SokolVertexAttr> &attrs,
									 bool usingIndices,
									 sg_primitive_type mode) {
	// Cache key hashes the full attribute layout (location/format/slot/step) plus
	// the index/blend/primitive state, so different attribute sets for the same
	// shader get distinct pipelines.
	size_t key = 0;
	auto mix   = [&key](size_t v) { key = key * 1000003u ^ v; };
	for (const auto &a: attrs) {
		mix(static_cast<size_t>(a.location));
		mix(static_cast<size_t>(a.format));
		mix(static_cast<size_t>(a.bufferSlot));
		mix(a.perInstance ? 1u : 0u);
		mix(static_cast<size_t>(a.offset));
		mix(static_cast<size_t>(a.bufferStride));
	}
	mix(usingIndices ? 1u : 0u);
	mix(static_cast<size_t>(g.getBlendMode()));
	mix(g.isBlending() ? 1u : 0u);
	mix(static_cast<size_t>(mode));

	// pipelines live in the process-global shader cache so instances share them
	auto &cache = ShaderCache::instance();
	std::lock_guard<std::mutex> l(cache.mut);
	auto &pipelines = static_cast<SharedShader *>(sharedEntry)->pipelines;
	auto it			= pipelines.find(key);
	if (it == pipelines.end()) {
		auto pipeline  = Pipeline::create(shd, attrs, usingIndices, g.isBlending(), g.getBlendMode(), mode);
		pipelines[key] = pipeline;
		return pipeline;
	}
	return it->second;
}
uint8_t *SokolShader::uniformLocation(const std::string &name) {
	int offset = def->uniformOffsetFunc(SG_SHADERSTAGE_VS, "vertParams", name.c_str());
	if (offset != -1) return vertUniforms.data() + offset;
	offset = def->uniformOffsetFunc(SG_SHADERSTAGE_FS, "fragParams", name.c_str());
	if (offset != -1) return fragUniforms.data() + offset;
	return nullptr;
}

void SokolShader::applyUniforms() {
	auto mvp = g.getMVP();
	memcpy(mvpUniformOffset, &mvp, sizeof(glm::mat4));
	if (colorUniformOffset != nullptr) {
		memcpy(colorUniformOffset, &g.getColor(), sizeof(glm::vec4));
	}

	if (!vertUniforms.empty()) sg_apply_uniforms(SG_SHADERSTAGE_VS, 0, &vertParamRange);
	if (!fragUniforms.empty()) sg_apply_uniforms(SG_SHADERSTAGE_FS, 0, &fragParamRange);
}
