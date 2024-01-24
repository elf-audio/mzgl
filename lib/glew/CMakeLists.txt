cmake_minimum_required (VERSION 3.18 FATAL_ERROR)

project (glew)

add_library (glew STATIC
             include/GL/eglew.h
             include/GL/glew.h
             include/GL/glxew.h
             include/GL/wglew.h
             src/glew.c
             src/glewinfo.c
             src/visualinfo.c)
target_include_directories (glew PUBLIC ../)
target_include_directories (glew PRIVATE include)