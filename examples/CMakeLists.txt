# for opengl or something
set(CMAKE_POLICY_DEFAULT_CMP0072 NEW)


cmake_minimum_required(VERSION 3.18)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_VERBOSE_MAKEFILE ON)

#Minimum MacOS target, set globally
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.9" CACHE STRING "Minimum OS X deployment version" FORCE)

project("examples")


add_compile_definitions(GL_SILENCE_DEPRECATION MZGL_GL2 MZGL_MAC_GLFW __MACOSX_CORE__)


add_subdirectory(".." "mzgl-build")
add_subdirectory(01_App)
add_subdirectory(02_Drawing)
add_subdirectory(03_Audio)
add_subdirectory(04_Dialogs)

