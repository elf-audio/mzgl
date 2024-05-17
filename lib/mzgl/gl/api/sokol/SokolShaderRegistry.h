#pragma once
#include "sokol_gfx.h"
#include <string>
#include "log.h"
#include "mzAssert.h"
typedef const sg_shader_desc *(*ShaderDescFn)(sg_backend backend);
typedef size_t (*UniformBlocksizeFn)(sg_shader_stage stage, const char *ub_name);
typedef int (*UniformOffsetFn)(sg_shader_stage stage, const char *ub_name, const char *u_name);
typedef int (*AttrSlotFn)(const char* attr_name);

class SokolAPI;
void registerShaders(SokolAPI &api);

class ShaderDef {
public:
	ShaderDef(const std::string &name,
			  ShaderDescFn descFunc,
			  UniformBlocksizeFn uniformblockSizeFunc,
			  UniformOffsetFn uniformOffsetFunc,
			  AttrSlotFn attrSlotFunc)
		: name(name)
		, descFunc(descFunc)
		, uniformblockSizeFunc(uniformblockSizeFunc)
		, uniformOffsetFunc(uniformOffsetFunc)
		, attrSlotFunc(attrSlotFunc){}
	std::string name;
	ShaderDescFn descFunc;
	UniformBlocksizeFn uniformblockSizeFunc;
	UniformOffsetFn uniformOffsetFunc;
	AttrSlotFn attrSlotFunc;
};

class SokolShaderRegistry {
public:
	SokolShaderRegistry()										= default;
	SokolShaderRegistry(const SokolShaderRegistry &)			= delete;
	SokolShaderRegistry &operator=(const SokolShaderRegistry &) = delete;

	void registerShader(const std::string &name,
						ShaderDescFn descFunc,
						UniformBlocksizeFn uniformblockSizeFunc,
						UniformOffsetFn uniformOffsetFunc,
						AttrSlotFn attrSlotFn) {
		// check a shader isn't already loaded with that name
		for (auto def: shaderRegistry) {
			if (def->name == name) {
				Log::e() << "Shader with the name '" << name << "' already exists!";
				return;
			}
		}
		shaderRegistry.emplace_back(
			std::make_shared<ShaderDef>(name, descFunc, uniformblockSizeFunc, uniformOffsetFunc, attrSlotFn));
	}

	std::shared_ptr<ShaderDef> getShaderDef(const std::string &name) const {
		for (auto def: shaderRegistry) {
			if (def->name == name) {
				return def;
			}
		}

		mzAssertFail("Couldn't find shader with name '" + name + "'");
		return nullptr;

	}

	std::vector<std::shared_ptr<ShaderDef>> shaderRegistry;
};