

#ifdef __APPLE__
#	ifndef GL_SILENCE_DEPRECATION
#		define GL_SILENCE_DEPRECATION
#	endif
#	ifndef GLES_SILENCE_DEPRECATION
#		define GLES_SILENCE_DEPRECATION
#	endif
#	include <TargetConditionals.h>
#	if TARGET_OS_IOS
#		include <OpenGLES/ES3/gl.h>
#	else

#		include <OpenGL/gl3.h>

#	endif
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
