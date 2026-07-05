#pragma once

// Normalizes the compile-time graphics-backend defines so that exactly one of
//
//     MZGL_SOKOL   - sokol_gfx (Metal on Apple, D3D11 on Windows, GLES3 elsewhere)
//     MZGL_METAL   - native Metal (Apple only)
//     MZGL_OPENGL  - GL 3.2+ / GLES3
//
// is defined after including this header. The build normally sets one of them
// (see cmake/GraphicsBackend.cmake); builds that predate the three-way switch
// and define nothing get OpenGL, as before.

#if defined(MZGL_SOKOL) && defined(MZGL_METAL)
#	error "MZGL_SOKOL and MZGL_METAL are mutually exclusive - pick one backend"
#endif
#if defined(MZGL_OPENGL) && (defined(MZGL_SOKOL) || defined(MZGL_METAL))
#	error "MZGL_OPENGL cannot be combined with another backend - pick one"
#endif

#if !defined(MZGL_SOKOL) && !defined(MZGL_METAL) && !defined(MZGL_OPENGL)
#	define MZGL_OPENGL 1
#endif

// MZGL_COMPILED_SHADERS: this backend consumes sokol-shdc-compiled shaders
// registered by name (Shader::create(g, "name")) rather than loading GLSL
// files at runtime. Normally supplied by the build (GraphicsBackend.cmake);
// derived here for builds that only set the backend macro.
#if (defined(MZGL_SOKOL) || defined(MZGL_METAL)) && !defined(MZGL_COMPILED_SHADERS)
#	define MZGL_COMPILED_SHADERS 1
#endif
