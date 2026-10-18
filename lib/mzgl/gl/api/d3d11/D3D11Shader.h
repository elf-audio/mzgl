#pragma once

// Shader for the native D3D11 backend. Compiles the sokol-shdc-generated HLSL
// (from the shared ShaderDef registry) with D3DCompile at first use, once per
// process per shader (a refcounted process-global cache, like MetalShader's),
// and mirrors SokolShader's CPU-side uniform-block scheme: uniforms are
// memcpy'd into byte buffers at reflection-resolved offsets and uploaded into
// one constant buffer per stage at draw time.
//
// Pure C++ header - the COM side lives in D3D11Shader.cpp behind an opaque
// cache-entry pointer.

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

class D3D11Shader : public Shader {
public:
	D3D11Shader(Graphics &g, const std::string &shaderName = "nothing");
	~D3D11Shader() override;

	void begin() override;
	void end() override;

	// MVP/color are pulled from Graphics at draw time (applyPipelineAndUniforms),
	// same as the Sokol backend
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

	// Called by D3D11Vbo (and the fontstash) at draw time, after the vertex
	// buffers are bound: resolves/caches the input layout for this attribute
	// layout, sets it plus the shaders, the constant buffers and the current
	// blend state on the immediate context (each memoised), and uploads the
	// uniform blocks. Returns false if the draw should be skipped (no frame,
	// shader failed to compile, layout mismatch).
	bool applyPipelineAndUniforms(const SokolVertexAttr *attrs, int numAttrs);

	std::vector<uint8_t> vertUniforms;
	std::vector<uint8_t> fragUniforms;
	std::string name;

private:
	uint8_t *uniformLocation(const std::string &name);
	std::shared_ptr<ShaderDef> def;

	// points at this shader's entry in the process-global compiled-shader
	// cache (see D3D11Shader.cpp); opaque here as the entry type is internal
	void *sharedEntry = nullptr;

	// memo of the last input layout used by this shader - draws almost
	// always repeat the same layout, so the common case skips the cache's
	// mutex. Unretained: the cache owns the layout and it lives as long as
	// the entry's compiled shader (both go on recompile/last-ref release).
	size_t lastLayoutKey		  = 0;
	void *lastLayout			  = nullptr;
	uint32_t lastLayoutGeneration = 0;

	uint8_t *colorUniformOffset = nullptr;
	uint8_t *mvpUniformOffset	= nullptr;

protected:
	void loadFromString(std::string vertCode, std::string fragCode) override {}
};
