//
//  Shader.h
//  MZGL
//
//  Created by Marek Bereza on 15/01/2018.
//  Copyright © 2018 Marek Bereza. All rights reserved.
//

#pragma once


#include "mzOpenGL.h"








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
	
	static ShaderRef create(Graphics &g) {
		return std::shared_ptr<Shader>(new Shader(g));
	}
	
	
	
	
	
	GLint positionAttribute = -1;
	GLint colorAttribute = -1;
	GLint texCoordAttribute = -1;
	GLint normAttribute = -1;
	
	GLuint shaderProgram = 0;
	bool isDefaultShader = false;
	
	// should be private in some way but vbo needs it because it manages shaders
	bool needsColorUniform = false;
	
//	static GLint getPositionAttribute() { return currShader->positionAttribute; }
//	static GLint getColorAttribute() { return currShader->colorAttribute; }
//	static GLint getTexCoordAttribute() { return currShader->texCoordAttribute; }
//	static GLint getNormalAttribute() { return currShader->normAttribute; }
	
	virtual ~Shader();
	
	void begin();
	void end();
	void uniform(std::string name, const glm::mat4 &m);
	
	void uniform(std::string name, int     		p);
	void uniform(std::string name, glm::ivec2  	p);
	
	
	void uniform(std::string name, float     p);
	void uniform(std::string name, glm::vec2 p);
	void uniform(std::string name, glm::vec3 p);
	void uniform(std::string name, glm::vec4 p);
	
	void uniform(std::string name, const std::vector<glm::mat4> &p);
	void uniform(std::string name, const std::vector<float> &p);
	void uniform(std::string name, const std::vector<glm::vec2> &p);
	void uniform(std::string name, const std::vector<glm::vec3> &p);
	void uniform(std::string name, const std::vector<glm::vec4> &p);
	
	
	void uniform(std::string name, const glm::mat4 *p, size_t length);
	void uniform(std::string name, const float     *p, size_t length);
	void uniform(std::string name, const glm::vec2 *p, size_t length);
	void uniform(std::string name, const glm::vec3 *p, size_t length);
	void uniform(std::string name, const glm::vec4 *p, size_t length);

	
	void loadFromString(std::string vertCode, std::string fragCode);
	
	void load(std::string vertex_file_path, std::string fragment_file_path);
	static std::string getVersionForPlatform(bool isVertShader);
	
#ifdef MZGL_GL2
	void setInstanceUniforms(int whichInstance);
#endif

#ifdef __ANDROID__
	static std::vector<Shader*> shaders;
#endif
	void deallocate();
private:
	void createProgram(GLuint vertexShader, GLuint fragmentShader);
	
	std::string readFile2(const std::string &fileName);
	GLuint compileShader(GLenum type, std::string src);
	
	void linkProgram(GLuint program);
	
	
	void validateProgram(GLuint program);
	
	
	
	// this is supposed to be for GL2 simulating instancing
	
#ifdef MZGL_GL2
	
	struct InstanceUniform {
		int dimensions = 0;
		std::vector<float> data;
		std::string name;
		InstanceUniform(std::string name, float *data, int numItems, int dimensions):
		name(name),
		dimensions(dimensions),
		data(data, data + numItems *dimensions) {}
	};
	std::vector<std::shared_ptr<InstanceUniform>> instanceUniforms;
#endif
	
};

