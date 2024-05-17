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
protected:
	Shader(Graphics &g);
	Shader(Graphics &g, std::string name);
	Graphics &g;

public:
	static ShaderRef create(Graphics &g);
	static ShaderRef create(Graphics &g, std::string name);

	bool isDefaultShader = false;

	virtual ~Shader();

	virtual void begin()											  = 0;
	virtual void end()												  = 0;
	virtual void uniform(const std::string &name, const glm::mat4 &m) = 0;

	virtual void setMVP(const glm::mat4 &mvp) = 0;
	virtual void setColor(const glm::vec4 &c) = 0;

	virtual void uniform(const std::string &name, int p)		= 0;
	virtual void uniform(const std::string &name, glm::ivec2 p) = 0;

	virtual void uniform(const std::string &name, float p)	   = 0;
	virtual void uniform(const std::string &name, glm::vec2 p) = 0;
	virtual void uniform(const std::string &name, glm::vec3 p) = 0;
	virtual void uniform(const std::string &name, glm::vec4 p) = 0;

	virtual void uniform(const std::string &name, const std::vector<glm::mat4> &p) = 0;
	virtual void uniform(const std::string &name, const std::vector<float> &p)	   = 0;
	virtual void uniform(const std::string &name, const std::vector<glm::vec2> &p) = 0;
	virtual void uniform(const std::string &name, const std::vector<glm::vec3> &p) = 0;
	virtual void uniform(const std::string &name, const std::vector<glm::vec4> &p) = 0;

	virtual void uniform(const std::string &name, const glm::mat4 *p, size_t length) = 0;
	virtual void uniform(const std::string &name, const float *p, size_t length)	 = 0;
	virtual void uniform(const std::string &name, const glm::vec2 *p, size_t length) = 0;
	virtual void uniform(const std::string &name, const glm::vec3 *p, size_t length) = 0;
	virtual void uniform(const std::string &name, const glm::vec4 *p, size_t length) = 0;

	virtual void loadFromString(std::string vertCode, std::string fragCode) = 0;

	virtual void load(const std::string &vertexFilePath, const std::string &fragFilePath) = 0;

	virtual void deallocate() = 0;

#ifdef __ANDROID__
	static std::vector<Shader *> shaders;
#endif
};
