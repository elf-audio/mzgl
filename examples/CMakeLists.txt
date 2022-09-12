
project("examples" CXX C)

# for opengl or something
set(CMAKE_POLICY_DEFAULT_CMP0072 NEW)
set(CMAKE_CXX_STANDARD 14)
cmake_minimum_required(VERSION 3.4.1)

add_compile_definitions(GL_SILENCE_DEPRECATION MZGL_GL2 MZGL_MAC_GLFW __MACOSX_CORE__)  

add_subdirectory(".." "mzgl-build")
add_subdirectory(01_App)
add_subdirectory(02_Drawing)
add_subdirectory(03_Audio)
add_subdirectory(04_Dialogs)

