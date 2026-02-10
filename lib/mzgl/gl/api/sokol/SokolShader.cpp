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
	vertUniforms.resize(def->uniformblockSizeFunc(SG_SHADERSTAGE_VS, "vertParams"), 0);
	fragUniforms.resize(def->uniformblockSizeFunc(SG_SHADERSTAGE_FS, "fragParams"), 0);

	vertParamRange = sg_range {vertUniforms.data(), vertUniforms.size()};
	fragParamRange = sg_range {fragUniforms.data(), fragUniforms.size()};

	colorUniformOffset = uniformLocation("color");
	mvpUniformOffset   = uniformLocation("mvp");
	mzAssert(mvpUniformOffset != nullptr, "mvp uniform not found in shader");
}

void SokolShader::begin() {
	g.currShader = this;
}
void SokolShader::end() {
	g.currShader = nullptr;
}

PipelineRef SokolShader::getPipeline(const std::vector<sg_vertex_format> &attrs,
									 bool usingIndices,
									 sg_primitive_type mode,
									 bool isInstancing) {
	// make a unique number to use as a key based on incoming parameters
	// (there will be at most 5-6 attributes)

	int index = static_cast<int>(attrs.size()) + usingIndices * 16 + static_cast<int>(g.getBlendMode()) * 32 + g.isBlending() * 64
				+ static_cast<int>(mode) * 128;

	if (pipelines.find(index) == pipelines.end()) {
		printf("creating pipeline %d (%s)\n", index, name.c_str());
		auto pipeline =
			Pipeline::create(shd, attrs, usingIndices, g.isBlending(), g.getBlendMode(), mode, isInstancing);
		pipelines[index] = pipeline;
		return pipeline;
	}
	return pipelines[index];
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
