#pragma once

#include <glm/glm.hpp>
using namespace glm;
#include "sokol_gfx.h"
#include <map>
#include <memory>
#include <string>
#include <cstring>
#include "Shader.h"
class Graphics;

class Pipeline;
using PipelineRef = std::shared_ptr<Pipeline>;

class ShaderDef;
class SokolShader : public Shader {
public:
	SokolShader(Graphics &g, const std::string &shaderName = "nothing");

	void begin() override;
	void end() override;

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

	void applyUniforms();

	sg_shader shd;
	template <typename T>
	void uniformSingle(const std::string &_name, const T &m) {
		auto *ptr = uniformLocation(_name);

		// TODO: Remove me!
		if (ptr == nullptr) {
			//	printf("Trying to set on a null shader def\n");
			return;
		}

		assert(ptr != nullptr);
		memcpy(ptr, &m, sizeof(T));
	}

	template <typename T>
	void uniformArray(const std::string &_name, const T *data, size_t count) {
		auto *ptr = uniformLocation(_name);

		// TODO: Remove me!
		if (ptr == nullptr) {
			//	printf("Trying to set on a null shader def\n");
			return;
		}

		assert(ptr != nullptr);
		memcpy(ptr, data, sizeof(T) * count);
	}

	std::map<int, PipelineRef> pipelines;

	PipelineRef getPipeline(const std::vector<sg_vertex_format> &attrs,
							bool usingIndices,
							sg_primitive_type mode,
							bool isInstancing);
	std::vector<uint8_t> vertUniforms;
	std::vector<uint8_t> fragUniforms;
	std::string name;

private:
	uint8_t *uniformLocation(const std::string &name);
	std::shared_ptr<ShaderDef> def;

	sg_range vertParamRange;
	sg_range fragParamRange;

	uint8_t *colorUniformOffset = nullptr;
	uint8_t *mvpUniformOffset	= nullptr;

protected:
	void loadFromString(std::string vertCode, std::string fragCode) override {}
};
