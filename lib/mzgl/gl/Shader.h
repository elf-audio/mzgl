//
//  Shader.h
//  MZGL
//
//  Created by Marek Bereza on 15/01/2018.
//  Copyright © 2018 Marek Bereza. All rights reserved.
//

#pragma once

#include <glm/glm.hpp>

#include <vector>
#include <string>
#include <memory>

#define STRINGIFY(s) #s
class Graphics;

class Shader;
typedef std::shared_ptr<Shader> ShaderRef;

class Shader {
private:
	Shader(Graphics &g);
	Graphics &g;

public:
	static ShaderRef create(Graphics &g) { return std::shared_ptr<Shader>(new Shader(g)); }

	int32_t positionAttribute = -1;
	int32_t colorAttribute	  = -1;
	int32_t texCoordAttribute = -1;
	int32_t normAttribute	  = -1;

	uint32_t shaderProgram = 0;

	bool isDefaultShader = false;

	// should be private in some way but vbo needs it because it manages shaders
	bool needsColorUniform = false;

	virtual ~Shader();

	void begin();
	void end();
	void uniform(std::string name, const glm::mat4 &m);

	void uniform(std::string name, int p);
	void uniform(std::string name, glm::ivec2 p);

	void uniform(std::string name, float p);
	void uniform(std::string name, glm::vec2 p);
	void uniform(std::string name, glm::vec3 p);
	void uniform(std::string name, glm::vec4 p);

	void uniform(std::string name, const std::vector<glm::mat4> &p);
	void uniform(std::string name, const std::vector<float> &p);
	void uniform(std::string name, const std::vector<glm::vec2> &p);
	void uniform(std::string name, const std::vector<glm::vec3> &p);
	void uniform(std::string name, const std::vector<glm::vec4> &p);

	void uniform(std::string name, const glm::mat4 *p, size_t length);
	void uniform(std::string name, const float *p, size_t length);
	void uniform(std::string name, const glm::vec2 *p, size_t length);
	void uniform(std::string name, const glm::vec3 *p, size_t length);
	void uniform(std::string name, const glm::vec4 *p, size_t length);

	void loadFromString(std::string vertCode, std::string fragCode);

	void load(std::string vertex_file_path, std::string fragment_file_path);
	static std::string getVersionForPlatform(bool isVertShader);

#ifdef __ANDROID__
	static std::vector<Shader *> shaders;
#endif
	void deallocate();

private:
	void createProgram(uint32_t vertexShader, uint32_t fragmentShader);

	std::string readFile2(const std::string &fileName);
	uint32_t compileShader(uint32_t type, std::string src);

	void linkProgram(uint32_t program);

	void validateProgram(uint32_t program);
};
