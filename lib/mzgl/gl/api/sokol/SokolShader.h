#pragma once

#include <glm/glm.hpp>
using namespace glm;
#include "sokol_gfx.h"
#include <map>
#include <memory>
#include <string>
#include "Shader.h"
class Graphics;

class Pipeline;
using PipelineRef = std::shared_ptr<Pipeline>;

class ShaderDef;
class SokolShader : public Shader {
public:
	//    Shader(Graphics &g) : g(g) {}
	SokolShader(Graphics &g, const std::string &shaderName = "nothing");

	void begin() override;
	void end() override;

	void setMVP(const glm::mat4 &mvp) override {}
	void setColor(const glm::vec4 &c) override {}
	void uniform(const std::string &name, const glm::mat4 &m) override { uniformSingle(name, m); }

	void uniform(const std::string &name, int p) override { uniformSingle(name, p); }
	void uniform(const std::string &name, glm::ivec2 p) override { uniformSingle(name, p); }

	void uniform(const std::string &name, float p) override { uniformSingle(name, p); }
	void uniform(const std::string &name, glm::vec2 p) override { uniformSingle(name, p); }
	void uniform(const std::string &name, glm::vec3 p) override { uniformSingle(name, p); }
	void uniform(const std::string &name, glm::vec4 p) override { uniformSingle(name, p); }
	void uniform(const std::string &name, const std::vector<glm::mat4> &p) override {
		uniformArray(name, p.data(), p.size());
	}
	void uniform(const std::string &name, const std::vector<float> &p) override {
		uniformArray(name, p.data(), p.size());
	}
	void uniform(const std::string &name, const std::vector<glm::vec2> &p) override {
		uniformArray(name, p.data(), p.size());
	}
	void uniform(const std::string &name, const std::vector<glm::vec3> &p) override {
		uniformArray(name, p.data(), p.size());
	}
	void uniform(const std::string &name, const std::vector<glm::vec4> &p) override {
		uniformArray(name, p.data(), p.size());
	}
	void uniform(const std::string &name, const glm::mat4 *p, size_t length) override {
		uniformArray(name, p, length);
	}

	void uniform(const std::string &name, const float *p, size_t length) override {
		uniformArray(name, p, length);
	}
	void uniform(const std::string &name, const glm::vec2 *p, size_t length) override {
		uniformArray(name, p, length);
	}
	void uniform(const std::string &name, const glm::vec3 *p, size_t length) override {
		uniformArray(name, p, length);
	}
	void uniform(const std::string &name, const glm::vec4 *p, size_t length) override {
		uniformArray(name, p, length);
	}

	void load(const std::string &vertexFilePath, const std::string &fragFilePath) override {}

	void deallocate() override {};

	void applyUniforms();

	sg_shader shd;
	template <typename T>
	void uniformSingle(const std::string &name, const T &m) {
		auto *ptr = uniformLocation(name);

		// TODO: Remove me!
		if (ptr == nullptr) {
			//	printf("Trying to set on a null shader def\n");
			return;
		}

		assert(ptr != nullptr);
		memcpy(ptr, &m, sizeof(T));
	}

	template <typename T>
	void uniformArray(const std::string &name, const T *data, int count) {
		auto *ptr = uniformLocation(name);

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
	//    void loadFromDesc(const std::string &shaderName, const sg_shader_desc &desc) {
	//        name = shaderName;
	//        shd = sg_make_shader(desc);
	//    }
	//    void loadFromString(bool isNothingShader, const std::string &vertSource, const std::string &fragSource) {
	//        sg_shader_desc shd_desc = {
	//                // The shader main() function cannot be called 'main' in
	//                // the Metal shader languages, thus we define '_main' as the
	//                // default function. This can be override with the
	//                // sg_shader_desc.vs.entry and sg_shader_desc.fs.entry fields.
	//                .vs.source = vertSource.c_str(),
	//                .fs.source = fragSource.c_str(),
	//                .vs.uniform_blocks[0].size = sizeof(VertParams),
	//                .fs.uniform_blocks[0].size = sizeof(Params),
	//        };
	//
	//
	//        // a shader pair, compiled from source code
	//        shd = sg_make_shader(shd_desc);
	//    }

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