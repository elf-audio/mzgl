//
// Created by Marek Bereza on 22/05/2024.
//

#include "OpenGLShader.h"
#include "Graphics.h"
#include "mzOpenGL.h"
#include "filesystem.h"
#include "stringUtil.h"

void OpenGLShader::begin() {
#ifdef MZGL_GL2
	instanceUniforms.clear();
#endif
	g.currShader = this;
	glUseProgram(shaderProgram);
}
void OpenGLShader::end() {
	g.currShader = nullptr;
	glUseProgram(0);
}
OpenGLShader::~OpenGLShader() {
	deallocate();
}

void OpenGLShader::setMVP(const glm::mat4 &mvp) {
	glUniformMatrix4fv(mvpLocation, 1, GL_FALSE, &mvp[0][0]);
}
void OpenGLShader::setColor(const glm::vec4 &c) {
	glUniform4fv(colorLocation, 1, (const GLfloat *) &c);
}
void OpenGLShader::deallocate() {
	if (shaderProgram != 0) {
		//		Log::e() << "Deleting shader " << shaderProgram;
		glDeleteProgram(shaderProgram);
		shaderProgram = 0;
	}
}
void OpenGLShader::uniform(const std::string &name, const glm::mat4 &m) {
	GLuint matId = glGetUniformLocation(shaderProgram, name.c_str());
	glUniformMatrix4fv(matId, 1, GL_FALSE, &m[0][0]);
}

void OpenGLShader::uniform(const std::string &name, glm::vec2 p) {
	GLuint vecId = glGetUniformLocation(shaderProgram, name.c_str());
	glUniform2fv(vecId, 1, (const GLfloat *) &p);
}

void OpenGLShader::uniform(const std::string &name, int p) {
	GLuint vecId = glGetUniformLocation(shaderProgram, name.c_str());
	glUniform1i(vecId, p);
}
void OpenGLShader::uniform(const std::string &name, glm::ivec2 p) {
	GLuint vecId = glGetUniformLocation(shaderProgram, name.c_str());
	glUniform2iv(vecId, 1, (const GLint *) &p);
}

void OpenGLShader::uniform(const std::string &name, float p) {
	GLuint vecId = glGetUniformLocation(shaderProgram, name.c_str());
	glUniform1f(vecId, p);
}

void OpenGLShader::uniform(const std::string &name, glm::vec3 p) {
	GLuint vecId = glGetUniformLocation(shaderProgram, name.c_str());
	glUniform3fv(vecId, 1, (const GLfloat *) &p);
}
void OpenGLShader::uniform(const std::string &name, glm::vec4 p) {
	GLuint vecId = glGetUniformLocation(shaderProgram, name.c_str());
	glUniform4fv(vecId, 1, (const GLfloat *) &p);
}

#ifdef MZGL_GL2
void OpenGLShader::setInstanceUniforms(int whichInstance) {
	for (auto i: instanceUniforms) {
		switch (i->dimensions) {
			case 1: uniform(i->name, i->data[whichInstance]); break;
			case 2: uniform(i->name, vec2(i->data[whichInstance * 2], i->data[whichInstance * 2 + 1])); break;
			case 3:
				uniform(i->name,
						vec3(i->data[whichInstance * 3],
							 i->data[whichInstance * 3 + 1],
							 i->data[whichInstance * 3 + 2]));
				break;

			case 4:
				uniform(i->name,
						vec4(i->data[whichInstance * 4],
							 i->data[whichInstance * 4 + 1],
							 i->data[whichInstance * 4 + 2],
							 i->data[whichInstance * 4 + 3]));
				break;
				// TODO: case for 16
		}
	}
}
#endif

void OpenGLShader::uniform(const std::string &name, const glm::mat4 *p, size_t length) {
#ifdef MZGL_GL2
	instanceUniforms.push_back(std::make_shared<InstanceUniform>(name, (float *) p, length, 16));
#else
	GLuint vecId = glGetUniformLocation(shaderProgram, name.c_str());
	glUniformMatrix4fv(vecId, (GLsizei) length, GL_FALSE, (const GLfloat *) p);
#endif
}

void OpenGLShader::uniform(const std::string &name, const float *p, size_t length) {
#ifdef MZGL_GL2
	instanceUniforms.push_back(std::make_shared<InstanceUniform>(name, (float *) p, length, 1));
#else
	GLuint vecId = glGetUniformLocation(shaderProgram, name.c_str());
	glUniform1fv(vecId, (GLsizei) length, (const GLfloat *) p);
#endif
}

void OpenGLShader::uniform(const std::string &name, const glm::vec2 *p, size_t length) {
#ifdef MZGL_GL2
	instanceUniforms.push_back(std::make_shared<InstanceUniform>(name, (float *) p, length, 2));
#else
	GLuint vecId = glGetUniformLocation(shaderProgram, name.c_str());
	glUniform2fv(vecId, (GLsizei) length, (const GLfloat *) p);
#endif
}

void OpenGLShader::uniform(const std::string &name, const glm::vec3 *p, size_t length) {
#ifdef MZGL_GL2
	instanceUniforms.push_back(std::make_shared<InstanceUniform>(name, (float *) p, length, 3));
#else
	GLuint vecId = glGetUniformLocation(shaderProgram, name.c_str());
	glUniform3fv(vecId, (GLsizei) length, (const GLfloat *) p);
#endif
}

void OpenGLShader::uniform(const std::string &name, const glm::vec4 *p, size_t length) {
#ifdef MZGL_GL2
	instanceUniforms.push_back(std::make_shared<InstanceUniform>(name, (float *) p, length, 4));
#else
	GLuint vecId = glGetUniformLocation(shaderProgram, name.c_str());
	glUniform4fv(vecId, (GLsizei) length, (const GLfloat *) p);
#endif
}

void OpenGLShader::uniform(const std::string &name, const std::vector<glm::vec4> &p) {
	uniform(name, p.data(), p.size());
}
void OpenGLShader::uniform(const std::string &name, const std::vector<float> &p) {
	uniform(name, p.data(), p.size());
}
void OpenGLShader::uniform(const std::string &name, const std::vector<glm::vec2> &p) {
	uniform(name, p.data(), p.size());
}
void OpenGLShader::uniform(const std::string &name, const std::vector<glm::vec3> &p) {
	uniform(name, p.data(), p.size());
}
void OpenGLShader::uniform(const std::string &name, const std::vector<glm::mat4> &p) {
	uniform(name, p.data(), p.size());
}

void OpenGLShader::loadFromString(std::string vertCode, std::string fragCode) {
	if (vertCode.find("#version") == -1) {
		vertCode = OpenGLShader::getVersionForPlatform(true) + vertCode;
	}

	if (fragCode.find("#version") == -1) {
		fragCode = OpenGLShader::getVersionForPlatform(false) + fragCode;
	}

	GLuint vertexShader	  = compileShader(GL_VERTEX_SHADER, vertCode);
	GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragCode);

	createProgram(vertexShader, fragmentShader);
}

void OpenGLShader::load(const std::string &vertexFilePath, const std::string &fragFilePath) {
	loadFromString(readFile2(vertexFilePath), readFile2(fragFilePath));
}

std::string OpenGLShader::getVersionForPlatform(bool isVertShader) {
#ifdef MZGL_GL2

	// convert GL3 shader to GL2 shader
	std::string version = "#version 120\n" // this is gl version 2.1
						  "#define lowp\n"
						  "#define highp\n"
						  "#define mediump\n"
						  "#define texture texture2D\n";
	if (isVertShader) {
		version += "#define out varying\n";
		version += "#define in attribute\n";
	} else {
		version += "#define in varying\n";
		version += "#define fragColor gl_FragColor\n";
	}
#else
	std::string version = "#version 150\n"; // this is GL version 3.2
#endif

#if TARGET_OS_IOS || defined(__ANDROID__) || defined(__arm__) || defined(USE_METALANGLE) || defined(__linux__)
	version = "#version 300 es\nprecision highp float;\n";
#endif
	return version;
}

static void linkProgram(GLuint program) {
	glLinkProgram(program);

	GLint logLength;

	glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);

	if (logLength > 0) {
		GLchar *log = (GLchar *) malloc((size_t) logLength);
		glGetProgramInfoLog(program, logLength, &logLength, log);

		Log::e() << "Shader program linking failed with error:\n" << log;
		free(log);
	}

	GLint status;
	glGetProgramiv(program, GL_LINK_STATUS, &status);
	if (0 == status) {
		Log::e() << "Failed to link shader program";
	}
}

void OpenGLShader::createProgram(GLuint vertexShader, GLuint fragmentShader) {
	if (0 != vertexShader && 0 != fragmentShader) {
		shaderProgram = glCreateProgram();

		glAttachShader(shaderProgram, vertexShader);
		glAttachShader(shaderProgram, fragmentShader);

#if !TARGET_OS_IOS && !defined(__ANDROID__) && !defined(__arm__) && defined(MZGL_GL3)
		// GL ES3 uses the built-in gl_FragCoord variable name for output,
		// OpenGL 3 on computer can have arbitarily defined outputs.
		glBindFragDataLocation(shaderProgram, 0, "fragColor");
#endif
		linkProgram(shaderProgram);

		colorAttribute	  = glGetAttribLocation(shaderProgram, "Color");
		texCoordAttribute = glGetAttribLocation(shaderProgram, "TexCoord");
		normAttribute	  = glGetAttribLocation(shaderProgram, "Normal");
		positionAttribute = glGetAttribLocation(shaderProgram, "Position");

		glDeleteShader(vertexShader);
		glDeleteShader(fragmentShader);

		// TODO: test for type here?
		needsColorUniform = glGetUniformLocation(shaderProgram, "color") != -1;
	} else {
		Log::e() << "Shader compilation failed.";
	}

	mvpLocation	  = glGetUniformLocation(shaderProgram, "mvp");
	colorLocation = glGetUniformLocation(shaderProgram, "color");
}

std::string OpenGLShader::readFile2(const std::string &fileName) {
	fs::ifstream ifs(fs::u8path(fileName.c_str()), std::ios::in | std::ios::binary | std::ios::ate);

	if (!ifs.good()) {
		Log::e() << "Error loading shader from " << fileName;
		return "";
	}
	std::ifstream::pos_type fileSize = ifs.tellg();
	ifs.seekg(0, std::ios::beg);

	std::vector<char> bytes(fileSize);
	ifs.read(bytes.data(), fileSize);

	return std::string(bytes.data(), fileSize);
}

#ifdef MZGL_GL2
static std::string removeSquareBrackets(const std::string &s) {
	std::string out;
	bool bracketOpen = false;
	for (int i = 0; i < s.size(); i++) {
		if (s[i] == '[') {
			bracketOpen = true;
		} else if (!bracketOpen) {
			out += s[i];
		}
		if (s[i] == ']') bracketOpen = false;
	}
	return out;
}
#endif

GLuint OpenGLShader::compileShader(GLenum type, std::string src) {
	std::string typeString = "unknown";
	if (type == GL_VERTEX_SHADER) typeString = "Vert";
	else if (type == GL_FRAGMENT_SHADER) typeString = "Frag";

	GLuint shader;
#ifdef MZGL_GL2
	replaceAll(src, "out vec4 fragColor;", "");
	// this removes any instance related stuff because
	// opengl 2 can't do instancing.
	src = removeSquareBrackets(src);
#endif
	const GLchar *source = (GLchar *) src.c_str();

	shader = glCreateShader(type);
	glShaderSource(shader, 1, &source, nullptr);
	glCompileShader(shader);

	GLint compileStatus;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &compileStatus);

	if (compileStatus == false) {
		GLint logLength;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);

		if (logLength > 1024 * 1024) {
			Log::e() << "Shader log length is huge, may be a nokia phone going crazy";
			logLength = 0;
		}
		if (logLength > 0) {
			GLchar *log = (GLchar *) malloc((size_t) logLength);
			glGetShaderInfoLog(shader, logLength, &logLength, log);

			std::string lm = log;

			// hide warning messages about implicit conversions or errors that aren't errors - some samsungs report an error of ' ' (just a space)
			if (lm.find("WARNING: 0:3: Overflow in implicit constant conversion") == -1 && lm.find("Success.") != 0
				&& lm != " ") {
				Log::e() << typeString << " shader compilation failed with error:\n'" << log << "'";
			}
			free(log);
			glDeleteShader(shader);
			Log::e() << typeString << "shader compilation failed for file";
		}
	}

	return shader;
}
