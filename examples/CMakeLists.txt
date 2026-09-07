# for opengl or something
set(CMAKE_POLICY_DEFAULT_CMP0072 NEW)


cmake_minimum_required(VERSION 3.18)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_VERBOSE_MAKEFILE ON)

# Minimum macOS target, set globally (11.0+ required for libc++'s C++20 <thread>
# (std::jthread / stop_token); 10.15 was the std::filesystem floor).
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum OS X deployment version" FORCE)

project("examples")


add_compile_definitions(GL_SILENCE_DEPRECATION __MACOSX_CORE__)


# Resolve the graphics backend here too (idempotent) so MZGL_BACKEND_COMPILE_DEFS
# is available to the plugin example, which is configured outside mzgl's own dir.
include(../cmake/GraphicsBackend.cmake)

add_subdirectory(".." "mzgl-build")
add_subdirectory(01_App)
add_subdirectory(02_Drawing)
add_subdirectory(03_Audio)
add_subdirectory(04_Dialogs)
add_subdirectory(05_Plugin)

