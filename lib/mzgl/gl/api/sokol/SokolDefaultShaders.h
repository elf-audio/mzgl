#pragma once

// this will be auto-generated

namespace nothing_shader {
#include "nothing.glsl.h"
};
namespace color_shader {
#include "color.glsl.h"
};
namespace colorTexture_shader {
#include "colorTexture.glsl.h"
};
namespace tex_shader {
#include "tex.glsl.h"
};
namespace font_shader {
#include "font.glsl.h"
};

// USER SHADERS ///////////////////////////////////////////////
//namespace flanger_shader {
//#include "flanger.glsl.h"
//};
///////////////////////////////////////////////////////////////

void registerShaders(SokolShaderRegistry &registry) {
	registry.registerShader("nothing",
							&nothing_shader::nothing_shader_desc,
							&nothing_shader::nothing_uniformblock_size,
							&nothing_shader::nothing_uniform_offset,
							&nothing_shader::nothing_attr_slot);
	registry.registerShader("color",
							&color_shader::color_shader_desc,
							&color_shader::color_uniformblock_size,
							&color_shader::color_uniform_offset,
							&color_shader::color_attr_slot);
	registry.registerShader("colorTexture",
							&colorTexture_shader::colorTexture_shader_desc,
							&colorTexture_shader::colorTexture_uniformblock_size,
							&colorTexture_shader::colorTexture_uniform_offset, &colorTexture_shader::colorTexture_attr_slot);
	registry.registerShader(
		"tex", &tex_shader::tex_shader_desc, &tex_shader::tex_uniformblock_size, &tex_shader::tex_uniform_offset, &tex_shader::tex_attr_slot);
	registry.registerShader("font",
							&font_shader::font_shader_desc,
							&font_shader::font_uniformblock_size,
							&font_shader::font_uniform_offset, &font_shader::font_attr_slot);
}