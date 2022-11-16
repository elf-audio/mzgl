//
//  Shader.cpp
//  MZGL
//
//  Created by Marek Bereza on 15/01/2018.
//  Copyright © 2018 Marek Bereza. All rights reserved.
//

#include "Shader.h"
#include "error.h"
#include <sstream>
#include <fstream>
#include <vector>
#include "log.h"
#include "stringUtil.h"
#include "Graphics.h"

using namespace std;

#define DEBUG 1

#ifdef __ANDROID__
vector<Shader*> Shader::shaders;
#endif


Shader::Shader(Graphics &g) : g(g) {
#ifdef __ANDROID__
    shaders.push_back(this);
#endif
}

void Shader::deallocate() {
	if(shaderProgram!=0) {
//		Log::e() << "Deleting shader " << shaderProgram;
		glDeleteProgram(shaderProgram);
		shaderProgram = 0;
	}
}

Shader::~Shader() {
	deallocate();

#ifdef __ANDROID__
	for(int i = 0; i < shaders.size(); i++) {
		if(shaders[i]==this) {
			shaders.erase(shaders.begin() + i);
			break;
		}
	}
#endif
}

void Shader::begin() {
#ifdef MZGL_GL2
	instanceUniforms.clear();
#endif
	
	g.currShader = this;
	GetError();
	glUseProgram(shaderProgram);
	GetError();
}
void Shader::end() {
	g.currShader = nullptr;
	GetError();
	glUseProgram(0);
	GetError();
}
void Shader::uniform(string name, const glm::mat4 &m) {
	GetError();
	GLuint matId = glGetUniformLocation(shaderProgram, name.c_str());
	GetError();
	glUniformMatrix4fv(matId, 1, GL_FALSE, &m[0][0]);
	GetError();
}

void Shader::uniform(string name, glm::vec2 p) {
	
	GLuint vecId = glGetUniformLocation(shaderProgram, name.c_str());
	GetError();

	glUniform2fv(vecId, 1, (const GLfloat *)&p);
	GetError();

}

void Shader::uniform(string name, int p) {
	GLuint vecId = glGetUniformLocation(shaderProgram, name.c_str());
	GetError();

	glUniform1i(vecId, p);
	GetError();
}
void Shader::uniform(string name, glm::ivec2 p) {
	
	GLuint vecId = glGetUniformLocation(shaderProgram, name.c_str());
	GetError();

	glUniform2iv(vecId, 1, (const GLint *)&p);
	GetError();

}

void Shader::uniform(string name, float    p) {
	GLuint vecId = glGetUniformLocation(shaderProgram, name.c_str());
	GetError();

	glUniform1f(vecId, p);
	GetError();
}

void Shader::uniform(string name, glm::vec3 p) {
	GLuint vecId = glGetUniformLocation(shaderProgram, name.c_str());
	GetError();

	glUniform3fv(vecId, 1, (const GLfloat *)&p);
	GetError();


}
void Shader::uniform(string name, glm::vec4 p) {
	GLuint vecId = glGetUniformLocation(shaderProgram, name.c_str());
	//GetError();

	glUniform4fv(vecId, 1, (const GLfloat *)&p);
	//GetError();
}


#ifdef MZGL_GL2
void Shader::setInstanceUniforms(int whichInstance) {
	for(auto i : instanceUniforms) {
		switch(i->dimensions) {
		case 1:
			uniform(i->name, i->data[whichInstance]);
			break;
		case 2:
			uniform(i->name, vec2(i->data[whichInstance*2], i->data[whichInstance*2+1]));
			break;
		case 3:
			uniform(i->name, vec3(i->data[whichInstance*3], i->data[whichInstance*3+1], i->data[whichInstance*3+2]));
			break;

		case 4:
			uniform(i->name, vec4(i->data[whichInstance*4], i->data[whichInstance*4+1],
								 i->data[whichInstance*4+2],i->data[whichInstance*4+3]));
			break;
		// TODO: case for 16
		}
	}
}
#endif


void Shader::uniform(string name, const glm::mat4 *p, size_t length) {
#ifdef MZGL_GL2
	instanceUniforms.push_back(make_shared<InstanceUniform>(name, (float*)p, length, 16));
#else
	GLuint vecId = glGetUniformLocation(shaderProgram, name.c_str());
	glUniformMatrix4fv(vecId, (GLsizei)length, GL_FALSE, (const GLfloat *)p);
#endif

}

void Shader::uniform(string name, const float     *p, size_t length) {
#ifdef MZGL_GL2
	instanceUniforms.push_back(make_shared<InstanceUniform>(name, (float*)p, length, 1));
#else
	GLuint vecId = glGetUniformLocation(shaderProgram, name.c_str());
	glUniform1fv(vecId, (GLsizei)length, (const GLfloat *)p);
#endif
}

void Shader::uniform(string name, const glm::vec2 *p, size_t length) {
#ifdef MZGL_GL2
		instanceUniforms.push_back(make_shared<InstanceUniform>(name, (float*)p, length, 2));
#else
		GLuint vecId = glGetUniformLocation(shaderProgram, name.c_str());
		glUniform2fv(vecId, (GLsizei)length, (const GLfloat *)p);
#endif
}

void Shader::uniform(string name, const glm::vec3 *p, size_t length) {
#ifdef MZGL_GL2
		instanceUniforms.push_back(make_shared<InstanceUniform>(name, (float*)p, length, 3));
#else
	GLuint vecId = glGetUniformLocation(shaderProgram, name.c_str());
	glUniform3fv(vecId, (GLsizei)length, (const GLfloat *)p);
#endif
}

void Shader::uniform(string name, const glm::vec4 *p, size_t length) {
#ifdef MZGL_GL2
	instanceUniforms.push_back(make_shared<InstanceUniform>(name, (float*)p, length, 4));
#else
	GLuint vecId = glGetUniformLocation(shaderProgram, name.c_str());
	glUniform4fv(vecId, (GLsizei)length, (const GLfloat *)p);
#endif
}




void Shader::uniform(string name, const vector<glm::vec4> &p) {
	uniform(name, p.data(), p.size());
}
void Shader::uniform(string name, const vector<float> &p) {
	uniform(name, p.data(), p.size());
}
void Shader::uniform(string name, const vector<glm::vec2> &p) {
	uniform(name, p.data(), p.size());
}
void Shader::uniform(string name, const vector<glm::vec3> &p) {
	uniform(name, p.data(), p.size());
}
void Shader::uniform(string name, const vector<glm::mat4> &p) {
	uniform(name, p.data(), p.size());
}



void Shader::loadFromString(string vertCode, string fragCode) {
	
	
	if(vertCode.find("#version")==-1) {
		vertCode = Shader::getVersionForPlatform(true) + vertCode;
	}
	
	if(fragCode.find("#version")==-1) {
		fragCode = Shader::getVersionForPlatform(false) + fragCode;
	}
	
	
	
	GLuint vertexShader   = compileShader(GL_VERTEX_SHADER,   vertCode);
	GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragCode);
	//printf("Trying to compile vert shader: \n%s\n", vertCode.c_str());
	//printf("Trying to compile frag shader: \n%s\n", fragCode.c_str());
	
	
	createProgram(vertexShader, fragmentShader);
	
}

void Shader::load(string vertex_file_path, string fragment_file_path) {
	loadFromString(readFile2(vertex_file_path), readFile2(fragment_file_path));
}


string Shader::getVersionForPlatform(bool isVertShader) {
#ifdef MZGL_GL2
	
	// convert GL3 shader to GL2 shader
	string version = "#version 120\n" // this is gl version 2.1
	"#define lowp\n"
	"#define highp\n"
	"#define mediump\n"
	"#define texture texture2D\n";
	if(isVertShader) {
		version += "#define out varying\n";
		version += "#define in attribute\n";
	} else {
		version += "#define in varying\n";
		version += "#define fragColor gl_FragColor\n";
	}
#else
	string version = "#version 150\n"; // this is GL version 3.2
#endif
	
	
#if TARGET_OS_IOS || defined(__ANDROID__) || defined(__arm__) || defined(USE_METALANGLE) || defined(__linux__)
        version = "#version 300 es\nprecision highp float;\n";
#endif
    return version;
}


void Shader::createProgram(GLuint vertexShader, GLuint fragmentShader) {
	
//	exit(0);
//	printf("bum\n");
	if (0 != vertexShader && 0 != fragmentShader) {
		shaderProgram = glCreateProgram();


//		if(shaderProgram==30 || shaderProgram==39) {
//		    Log::d() << "I'm the problem";
//		}
//		Log::d() << "Created shader " << shaderProgram;
		GetError();
		
		glAttachShader(shaderProgram, vertexShader  );
		GetError();
		glAttachShader(shaderProgram, fragmentShader);
		GetError();
#if !TARGET_OS_IOS && !defined(__ANDROID__) && !defined(__arm__) && defined(MZGL_GL3)
		// GL ES3 uses the built-in gl_FragCoord variable name for output,
		// OpenGL 3 on computer can have arbitarily defined outputs.
		glBindFragDataLocation(shaderProgram, 0, "fragColor");
#endif
		linkProgram(shaderProgram);
		
		GetError();
		
		colorAttribute = glGetAttribLocation(shaderProgram, "Color");
		GetError();

		if (colorAttribute < 0) {
			//Log::e() << "Shader did not contain the 'color' attribute.";
		}
		
		texCoordAttribute = glGetAttribLocation(shaderProgram, "TexCoord");
		GetError();

		if (texCoordAttribute < 0) {
			//Log::e() << "Shader did not contain the 'texCoord' attribute.";
		}
		
		normAttribute = glGetAttribLocation(shaderProgram, "Normal");
		GetError();
		if (normAttribute < 0) {
			//Log::e() << "Shader did not contain the 'Normal' attribute.";
		}
		
		positionAttribute = glGetAttribLocation(shaderProgram, "Position");
		GetError();
		if (positionAttribute < 0) {
			//Log::e() << "Shader did not contain the 'position' attribute.";
		}
		
		glDeleteShader(vertexShader  );
		GetError();
		glDeleteShader(fragmentShader);
		GetError();
		
		// TODO: test for type here?
		needsColorUniform = glGetUniformLocation(shaderProgram, "color")!=-1;
	} else {
		Log::e() << "Shader compilation failed.";
	}
}


string Shader::readFile2(const string &fileName)
{
	ifstream ifs(fileName.c_str(), ios::in | ios::binary | ios::ate);
	
	if(!ifs.good()) {
		Log::e() << "Error loading shader from " << fileName;
		return "";
	}
	ifstream::pos_type fileSize = ifs.tellg();
	ifs.seekg(0, ios::beg);
	
	vector<char> bytes(fileSize);
	ifs.read(bytes.data(), fileSize);
	
	return string(bytes.data(), fileSize);
}


#ifdef MZGL_GL2
string removeSquareBrackets(const string &s) {
	string out = "";
	bool bracketOpen = false;
	for(int i = 0; i < s.size(); i++) {
		if(s[i]=='[') {
			bracketOpen = true;
		} else if(!bracketOpen) {
			out += s[i];
		}
		if(s[i]==']') bracketOpen = false;
	}
	return out;
}
#endif

GLuint Shader::compileShader(GLenum type, string src) {
	
	string typeString = "unknown";
	if(type==GL_VERTEX_SHADER) typeString = "Vert";
	else if(type==GL_FRAGMENT_SHADER) typeString = "Frag";
	
	GLuint shader;
#ifdef MZGL_GL2
	replaceAll(src, "out vec4 fragColor;", "");
	// this removes any instance related stuff because
	// opengl 2 can't do instancing.
	src = removeSquareBrackets(src);
#endif
	const GLchar *source = (GLchar*)src.c_str();

	
	if (NULL == source) {
		//[NSException raise:@"FAILED" format:@"Failed to read shader file %s", file.c_str()];
		// TODO: proper warning
		Log::e() << "Failed to read shader text";
		//throw 0;
	}
	
	shader = glCreateShader(type);
	GetError();
	glShaderSource(shader, 1, &source, NULL);
	GetError();
	glCompileShader(shader);
	GetError();
	
//#if defined(DEBUG)
	GLint logLength;
	
	glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
	GetError();
	if (logLength > 0) {
		GLchar *log = (GLchar*)malloc((size_t)logLength);
		glGetShaderInfoLog(shader, logLength, &logLength, log);
		GetError();
		std::string lm = log;
		
		// hide warning messages for now
		if(lm.find("WARNING: 0:3: Overflow in implicit constant conversion")==-1) {
			Log::e() << typeString << " shader compilation failed with error:\n" << log;
		}
		free(log);
	}
//#endif
	
	GLint status;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
	GetError();
	if (0 == status) {
		glDeleteShader(shader);
		GetError();
		Log::e() << typeString << "shader compilation failed for file";
		//throw 0;
		//[NSException raise:@"FAILED" format:@"Shader compilation failed for file %@", file];
	}
	return shader;
}

void Shader::linkProgram(GLuint program) {
	glLinkProgram(program);
	GetError();
	
#if defined(DEBUG)
	GLint logLength;
	
	glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
	GetError();
	if (logLength > 0)
	{
		GLchar *log = (GLchar*)malloc((size_t)logLength);
		glGetProgramInfoLog(program, logLength, &logLength, log);
		GetError();
		Log::e() << "Shader program linking failed with error:\n" << log;
		free(log);
	}
#endif
	
	GLint status;
	glGetProgramiv(program, GL_LINK_STATUS, &status);
	GetError();
	if (0 == status)
	{
		//[NSException raise:@"Failed" format:@"Failed to link shader program"];
		Log::e() << "Failed to link shader program";
		//throw 0;
	}
}

void Shader::validateProgram(GLuint program) {
	GLint logLength;
	
	glValidateProgram(program);
	GetError();
	glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
	GetError();
	if (logLength > 0)
	{
		GLchar *log = (GLchar*)malloc((size_t)logLength);
		glGetProgramInfoLog(program, logLength, &logLength, log);
		GetError();
		Log::e() << "Program validation produced errors:\n" << log;
		free(log);
	}
	
	GLint status;
	glGetProgramiv(program, GL_VALIDATE_STATUS, &status);
	GetError();
	if (0 == status)
	{
		Log::e() << "Failed to link shader program";
		//throw 0;
		//[NSException raise:@"Failed " format:@"Failed to link shader program"];
	}
}


