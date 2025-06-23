//
//  Shader.cpp
//  MZGL
//
//  Created by Marek Bereza on 15/01/2018.
//  Copyright © 2018 Marek Bereza. All rights reserved.
//

#include "Shader.h"
#include "Graphics.h"
#include "SokolShader.h"
#include "OpenGLShader.h"

ShaderRef Shader::create(Graphics &g) {
#ifdef MZGL_SOKOL_METAL
	return std::shared_ptr<Shader>(new SokolShader(g));
#else
	return std::shared_ptr<Shader>(new OpenGLShader(g));
#endif
}
ShaderRef Shader::create(Graphics &g, std::string name) {
#ifdef MZGL_SOKOL_METAL
	return std::shared_ptr<Shader>(new SokolShader(g, name));
#else
	return std::shared_ptr<Shader>(new OpenGLShader(g, name));
#endif
}

Shader::Shader(Graphics &g)
	: g(g) {
#ifdef __ANDROID__
	shaders.push_back(this);
#endif
}
Shader::Shader(Graphics &g, std::string name)
	: Shader(g) {
	// TODO: creating shader with name, so this is a Sokol shader
}

#ifdef __ANDROID__
std::vector<Shader *> Shader::shaders;
#endif

Shader::~Shader() {
#ifdef __ANDROID__
	for (int i = 0; i < shaders.size(); i++) {
		if (shaders[i] == this) {
			shaders.erase(shaders.begin() + i);
			break;
		}
	}
#endif
}
