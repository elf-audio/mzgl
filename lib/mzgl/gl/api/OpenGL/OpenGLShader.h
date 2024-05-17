//
// Created by Marek Bereza on 22/05/2024.
//

#pragma once
#include "Shader.h"

class OpenGLShader : public Shader {
public:
	OpenGLShader(Graphics &g)
		: Shader(g) {}
	OpenGLShader(Graphics &g, std::string name)
		: Shader(g) {}
	~OpenGLShader() override;
#ifdef MZGL_GL2
	void setInstanceUniforms(int whichInstance);
#endif

	// should be private in some way but vbo needs it because it manages shaders
	bool needsColorUniform = false;

	int32_t positionAttribute = -1;
	int32_t colorAttribute	  = -1;
	int32_t texCoordAttribute = -1;
	int32_t normAttribute	  = -1;

	uint32_t shaderProgram = 0;

	static std::string getVersionForPlatform(bool isVertShader);

	void begin() override;
	void end() override;
	void uniform(const std::string &name, const glm::mat4 &m) override;

	void setMVP(const glm::mat4 &mvp) override;
	void setColor(const glm::vec4 &c) override;

	void uniform(const std::string &name, int p) override;
	void uniform(const std::string &name, glm::ivec2 p) override;

	void uniform(const std::string &name, float p) override;
	void uniform(const std::string &name, glm::vec2 p) override;
	void uniform(const std::string &name, glm::vec3 p) override;
	void uniform(const std::string &name, glm::vec4 p) override;

	void uniform(const std::string &name, const std::vector<glm::mat4> &p) override;
	void uniform(const std::string &name, const std::vector<float> &p) override;
	void uniform(const std::string &name, const std::vector<glm::vec2> &p) override;
	void uniform(const std::string &name, const std::vector<glm::vec3> &p) override;
	void uniform(const std::string &name, const std::vector<glm::vec4> &p) override;

	void uniform(const std::string &name, const glm::mat4 *p, size_t length) override;
	void uniform(const std::string &name, const float *p, size_t length) override;
	void uniform(const std::string &name, const glm::vec2 *p, size_t length) override;
	void uniform(const std::string &name, const glm::vec3 *p, size_t length) override;
	void uniform(const std::string &name, const glm::vec4 *p, size_t length) override;

	void loadFromString(std::string vertCode, std::string fragCode) override;

	void load(const std::string &vertexFilePath, const std::string &fragFilePath) override;

	void deallocate() override;

private:
	void createProgram(uint32_t vertexShader, uint32_t fragmentShader);

	std::string readFile2(const std::string &fileName);
	uint32_t compileShader(uint32_t type, std::string src);

	uint32_t mvpLocation;
	uint32_t colorLocation;
	// this is supposed to be for GL2 simulating instancing

#ifdef MZGL_GL2

	struct InstanceUniform {
		int dimensions = 0;
		std::vector<float> data;
		std::string name;
		InstanceUniform(std::string name, float *data, int numItems, int dimensions)
			: name(name)
			, dimensions(dimensions)
			, data(data, data + numItems * dimensions) {}
	};
	std::vector<std::shared_ptr<InstanceUniform>> instanceUniforms;
#endif
};
