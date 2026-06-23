#pragma once

#include "sokol_gfx.h"
#include "sokol_log.h"

// MSAA sample count. Three things per backend must agree on this: the surface
// being rendered to (MTKView / D3D11 render target / WebGL framebuffer via
// GLFW_SAMPLES), the sg_environment defaults (which size the pipelines), and
// the per-frame sg_pass swapchain.
constexpr int mzglSokolSampleCount = 4;

// One central sg_setup() for all backends (Metal, D3D11/GLFW, GLES3/emscripten)
// so the logger and pool sizes only live in one place - callers just supply
// the backend-specific sg_environment.
inline void mzglSokolSetup(const sg_environment &environment) {
	// one global instance of sokol 
	if (sg_isvalid()) return;

	sg_desc desc			= {};
	desc.environment		= environment;
	desc.logger.func		= slog_func;
	desc.buffer_pool_size	= 4096; // sokol default is 128
	desc.shader_pool_size	= 128; // sokol default is 32
	desc.pipeline_pool_size = 512; // sokol default of 64 is too small (one pipeline per shader/blend/primitive combo)
	desc.image_pool_size	= 256; // sokol default of 128 is too small - font atlases, AUv3 plugin icons,
									// waveform/sample textures all consume image slots at runtime. Exhausting the
									// pool makes sg_make_image return SG_INVALID_ID and textures render as black rects.
									// 256 is comfortably above peak concurrent images; the pool is just CPU-side
									// handle bookkeeping (~100 bytes/slot), no GPU/texture memory.
	sg_setup(desc);
}
