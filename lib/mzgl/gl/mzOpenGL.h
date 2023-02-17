

#ifdef __APPLE__
#   include <TargetConditionals.h>
#	ifdef USE_METALANGLE
#		include <MetalANGLE/GLES3/gl32.h>
#	else
#   	if TARGET_OS_IOS
#       	include <OpenGLES/ES3/gl.h>
#   	else
#			ifdef MZGL_GL2
#				include <OpenGL/gl.h>
#				define glGenVertexArrays glGenVertexArraysAPPLE
#				define glBindVertexArray glBindVertexArrayAPPLE
#				define glDeleteVertexArrays glDeleteVertexArraysAPPLE
#			else
#       		include <OpenGL/gl3.h>
#			endif
#		endif
#   endif
#elif defined(_WIN32)
#	include <Windows.h>

#	include "glew.h"
#	include "glfw3native.h"

#else

#	if __ANDROID_API__ >= 24
#		include <GLES3/gl32.h>
#	elif __ANDROID_API__ >= 21
#		include <GLES3/gl31.h>
#	else
#		ifdef RPI // I'm not sure if this macro works to choose raspberry pi
#			include <GLES3/gl3.h>
#		else
#			define GL_GLEXT_PROTOTYPES
#			include <GL/gl.h>
#			include <GL/glut.h>
#		endif
#	endif
#endif
