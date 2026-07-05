#pragma once

// Shader for the native Metal backend. Compiles the sokol-shdc-generated MSL
// (from the shared ShaderDef registry) into an MTLLibrary at first use, and
// mirrors SokolShader's CPU-side uniform-block scheme: uniforms are memcpy'd
// into byte buffers at reflection-resolved offsets and uploaded with
// setVertexBytes/setFragmentBytes at draw time.
//
// Pure C++ header - Objective-C lives in MetalShader.mm behind an opaque
// cache-entry pointer (same pattern as SokolShader's process-global cache).

#include <glm/glm.hpp>
using namespace glm;
#include "sokol_gfx.h"
#include "SokolVertexAttr.h"
#include <map>
#include <memory>
#include <string>
#include <cstring>
#include <vector>
#include "Shader.h"

class Graphics;
class ShaderDef;

class MetalShader : public Shader {
public:
	MetalShader(Graphics &g, const std::string &shaderName = "nothing");
	~MetalShader() override;

	void begin() override;
	void end() override;

	// MVP/color are pulled from Graphics at draw time (applyUniforms), same
	// as the Sokol backend
	void setMVP(const glm::mat4 &mvp) override {}
	void setColor(const glm::vec4 &c) override {}
	void uniform(const std::string &_name, const glm::mat4 &m) override { uniformSingle(_name, m); }

	void uniform(const std::string &_name, int p) override { uniformSingle(_name, p); }
	void uniform(const std::string &_name, glm::ivec2 p) override { uniformSingle(_name, p); }

	void uniform(const std::string &_name, float p) override { uniformSingle(_name, p); }
	void uniform(const std::string &_name, glm::vec2 p) override { uniformSingle(_name, p); }
	void uniform(const std::string &_name, glm::vec3 p) override { uniformSingle(_name, p); }
	void uniform(const std::string &_name, glm::vec4 p) override { uniformSingle(_name, p); }
	void uniform(const std::string &_name, const std::vector<glm::mat4> &p) override {
		uniformArray(_name, p.data(), p.size());
	}
	void uniform(const std::string &_name, const std::vector<float> &p) override {
		uniformArray(_name, p.data(), p.size());
	}
	void uniform(const std::string &_name, const std::vector<glm::vec2> &p) override {
		uniformArray(_name, p.data(), p.size());
	}
	void uniform(const std::string &_name, const std::vector<glm::vec3> &p) override {
		uniformArray(_name, p.data(), p.size());
	}
	void uniform(const std::string &_name, const std::vector<glm::vec4> &p) override {
		uniformArray(_name, p.data(), p.size());
	}
	void uniform(const std::string &_name, const glm::mat4 *p, size_t length) override {
		uniformArray(_name, p, length);
	}

	void uniform(const std::string &_name, const float *p, size_t length) override {
		uniformArray(_name, p, length);
	}
	void uniform(const std::string &_name, const glm::vec2 *p, size_t length) override {
		uniformArray(_name, p, length);
	}
	void uniform(const std::string &_name, const glm::vec3 *p, size_t length) override {
		uniformArray(_name, p, length);
	}
	void uniform(const std::string &_name, const glm::vec4 *p, size_t length) override {
		uniformArray(_name, p, length);
	}

	void load(const std::string &vertexFilePath, const std::string &fragFilePath) override {}

	void deallocate() override {};

	template <typename T>
	void uniformSingle(const std::string &_name, const T &m) {
		auto *ptr = uniformLocation(_name);
		if (ptr == nullptr) return;
		memcpy(ptr, &m, sizeof(T));
	}

	template <typename T>
	void uniformArray(const std::string &_name, const T *data, size_t count) {
		auto *ptr = uniformLocation(_name);
		if (ptr == nullptr) return;
		memcpy(ptr, data, sizeof(T) * count);
	}

	// Shader attribute location for a vertex attribute by name (e.g.
	// "Position", "Color", "TexCoord"), from the sokol-shdc reflection.
	// Returns -1 if the shader doesn't use that attribute.
	int attrSlot(const std::string &attrName) const;

	// Called by MetalVbo (and the fontstash) at draw time: resolves/caches a
	// MTLRenderPipelineState for this attribute layout + the current blend
	// state, sets it on the frame's encoder, and uploads the uniform blocks.
	// Returns false if the pipeline couldn't be built (draw should be skipped).
	bool applyPipelineAndUniforms(const SokolVertexAttr *attrs, int numAttrs);

	std::vector<uint8_t> vertUniforms;
	std::vector<uint8_t> fragUniforms;
	std::string name;

private:
	uint8_t *uniformLocation(const std::string &name);
	std::shared_ptr<ShaderDef> def;

	// points at this shader's entry in the process-global compiled-shader
	// cache (see MetalShader.mm); opaque here as the entry type is internal
	void *sharedEntry = nullptr;

	// memo of the last pipeline used by this shader - draws almost always
	// repeat the same layout/blend key, so the common case skips the shared
	// cache's mutex entirely. Unretained: the cache owns the pipeline and
	// outlives this shader (entries are only erased when the last shader
	// referencing them is destroyed).
	size_t lastPipelineKey = 0;
	void *lastPipeline	   = nullptr;

	uint8_t *colorUniformOffset = nullptr;
	uint8_t *mvpUniformOffset	= nullptr;

protected:
	void loadFromString(std::string vertCode, std::string fragCode) override {}
};
