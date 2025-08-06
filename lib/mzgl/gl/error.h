/*
 **  error.h
 **
 **  Copyright (c) 2008 Max Rupp (feelgood@cs.pdx.edu) All rights reserved.
 */

#pragma once
// take out underscores to reenable proper erroring

#ifdef DEBUG

#	include <stdlib.h>
#	include <assert.h>
#	include "log.h"
#	if defined(_WIN32)
#		include <intrin.h>
#		define DEBUG_BREAK() __debugbreak()
#	else
#		include <csignal>
#		define DEBUG_BREAK() std::raise(SIGTRAP)
#	endif

#	include "mzOpenGL.h"

extern bool alreadyHadGLError;
#	define GetError()                                                                                            \
		{                                                                                                         \
			for (GLenum Error = glGetError(); (GL_NO_ERROR != Error); Error = glGetError()) {                     \
				if (alreadyHadGLError) break;                                                                     \
				alreadyHadGLError = true;                                                                         \
				switch (Error) {                                                                                  \
					case GL_INVALID_ENUM: Log::e() << "GL_INVALID_ENUM - " << __FILE__ << ": " << __LINE__;       \
						break;                                                                                    \
					case GL_INVALID_VALUE:                                                                        \
						Log::e() << "GL_INVALID_VALUE - " << __FILE__ << ": " << __LINE__;                        \
						break;                                                                                    \
					case GL_INVALID_OPERATION:                                                                    \
						Log::e() << "GL_INVALID_OPERATION - " << __FILE__ << ": " << __LINE__;                    \
						break;                                                                                    \
					case GL_OUT_OF_MEMORY:                                                                        \
						Log::e() << "GL_OUT_OF_MEMORY - " << __FILE__ << ": " << __LINE__;                        \
						break;                                                                                    \
					default:                                                                                      \
						Log::e() << "Unkown GL Error - " << __FILE__ << ": " << __LINE__ << " - " << Error;       \
						break;                                                                                    \
				}                                                                                                 \
				DEBUG_BREAK();                                                                                    \
			}                                                                                                     \
		}

#	define CheckFramebufferStatus()                                                                              \
		{                                                                                                         \
			switch (glCheckFramebufferStatus(GL_FRAMEBUFFER)) {                                                   \
				case GL_FRAMEBUFFER_COMPLETE: return;                                                             \
				case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:                                                        \
					Log::e() << "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT - " << __FILE__ << ": " << __LINE__;        \
					break;                                                                                        \
				case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:                                                \
					Log::e() << "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT - " << __FILE__ << ": "             \
							 << __LINE__;                                                                         \
					break;                                                                                        \
				case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:                                                       \
					Log::e() << "GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER - " << __FILE__ << ": " << __LINE__;       \
					break;                                                                                        \
				case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:                                                       \
					Log::e() << "GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER - " << __FILE__ << ": " << __LINE__;       \
					break;                                                                                        \
				case GL_FRAMEBUFFER_UNSUPPORTED:                                                                  \
					Log::e() << "GL_FRAMEBUFFER_UNSUPPORTED - " << __FILE__ << ": " << __LINE__;                  \
					break;                                                                                        \
				case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:                                                       \
					Log::e() << "GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE - " << __FILE__ << ": " << __LINE__;       \
					break;                                                                                        \
				case GL_FRAMEBUFFER_UNDEFINED:                                                                    \
					Log::e() << "GL_FRAMEBUFFER_UNDEFINED - " << __FILE__ << ": " << __LINE__;                    \
					break;                                                                                        \
				default: break;                                                                                   \
			}                                                                                                     \
			DEBUG_BREAK();                                                                                        \
		}

#	define GetShaderInfoLog(Shader, Source)                                                                      \
		{                                                                                                         \
			GLint Status, Count;                                                                                  \
			GLchar *Error;                                                                                        \
			glGetShaderiv(Shader, GL_COMPILE_STATUS, &Status);                                                    \
                                                                                                                  \
			if (!Status) {                                                                                        \
				glGetShaderiv(Shader, GL_INFO_LOG_LENGTH, &Count);                                                \
				if (Count > 0) {                                                                                  \
					glGetShaderInfoLog(Shader, Count, NULL, (Error = calloc(1, Count)));                          \
					Log::e() << Source << "\n\n" << Error;                                                        \
					free(Error);                                                                                  \
					assert(0);                                                                                    \
				}                                                                                                 \
			}                                                                                                     \
		}

#else

#	define GetError()
#	define CheckFramebufferStatus()
#	define GetShaderInfoLog(Shader, Source)

#endif //   DEBUG
