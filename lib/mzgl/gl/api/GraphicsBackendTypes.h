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
