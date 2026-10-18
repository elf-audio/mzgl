#pragma once

// Maps the compile-time backend choice onto the concrete implementation
// types. This is the only place that knows which classes implement the
// GraphicsAPI/Shader/Vbo/Texture interfaces - include it from the facade
// .cpp files (Graphics.cpp / Shader.cpp / Vbo.cpp / Texture.cpp) only;
// everything else should stay backend-agnostic.

#include "backendDefines.h"

#if defined(MZGL_SOKOL)

#	include "SokolAPI.h"
#	include "SokolShader.h"
#	include "SokolVbo.h"
#	include "SokolTexture.h"

using BackendGraphicsAPI = SokolAPI;
using BackendShader		 = SokolShader;
using BackendVbo		 = SokolVbo;
using BackendTexture	 = SokolTexture;

#elif defined(MZGL_METAL)

#	include "MetalAPI.h"
#	include "MetalShader.h"
#	include "MetalVbo.h"
#	include "MetalTexture.h"

using BackendGraphicsAPI = MetalAPI;
using BackendShader		 = MetalShader;
using BackendVbo		 = MetalVbo;
using BackendTexture	 = MetalTexture;

#elif defined(MZGL_D3D11)

#	include "D3D11API.h"
#	include "D3D11Shader.h"
#	include "D3D11Vbo.h"
#	include "D3D11Texture.h"

using BackendGraphicsAPI = D3D11API;
using BackendShader		 = D3D11Shader;
using BackendVbo		 = D3D11Vbo;
using BackendTexture	 = D3D11Texture;

#else // MZGL_OPENGL

#	include "OpenGLAPI.h"
#	include "OpenGLShader.h"
#	include "OpenGLVbo.h"
#	include "OpenGLTexture.h"

using BackendGraphicsAPI = OpenGLAPI;
using BackendShader		 = OpenGLShader;
using BackendVbo		 = OpenGLVbo;
using BackendTexture	 = OpenGLTexture;

#endif
