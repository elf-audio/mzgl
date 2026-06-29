#include "Graphics.h"
#include "SokolPipeline.h"
#include "SokolAPI.h"
#include "mzgl_platform.h"
CLANG_IGNORE_WARNINGS_BEGIN("-Wshorten-64-to-32")
#include "SokolShader.h"
CLANG_IGNORE_WARNINGS_END

SokolShader::SokolShader(Graphics &g, const std::string &shaderName)
	: Shader(g) {
	name	  = shaderName;
	auto *api = static_cast<SokolAPI *>(&g.getAPI());
	def		  = api->getShaderDef(shaderName);
	shd		  = sg_make_shader(def->descFunc(sg_query_backend()));
	if (shd.id == SG_INVALID_ID) {
		Log::e() << "sg_make_shader failed for '" << shaderName << "' - shader pool exhausted?";
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
	// destroy the pipelines first - they reference the shader
	pipelines.clear();
	// sg_isvalid() guard: the layer tree can outlive sg_shutdown() on close
	if (sg_isvalid() && shd.id != SG_INVALID_ID) {
		sg_destroy_shader(shd);
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

	if (pipelines.find(key) == pipelines.end()) {
		auto pipeline = Pipeline::create(shd, attrs, usingIndices, g.isBlending(), g.getBlendMode(), mode);
		pipelines[key] = pipeline;
		return pipeline;
	}
	return pipelines[key];
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
