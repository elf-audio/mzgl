cmake_minimum_required (VERSION 3.18 FATAL_ERROR)

project (mzgl VERSION 1.0.0)

include (cmake/print.cmake)

include (cmake/build-flags.cmake)
include (cmake/cpm.cmake)
include (cmake/dependencies.cmake)
include (cmake/flags.cmake)
include (cmake/libraries.cmake)
include (cmake/policies.cmake)
include (cmake/platforms.cmake)
include (cmake/properties.cmake)
include (cmake/sources.cmake)

if ("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_CURRENT_SOURCE_DIR}")
  mzgl_download_cpm ()
endif ()
mzgl_add_packages ()

add_subdirectory (lib/elfxml)
add_subdirectory (lib/fsystem)
add_subdirectory (lib/glew)
add_subdirectory (lib/ZipFile)

add_library (mzgl STATIC ${MZGL_SOURCES})
mzgl_set_target_properties (mzgl)
target_include_directories (
  mzgl
  PUBLIC lib/mzgl
         lib/mzgl/animation
         lib/mzgl/app/android
         lib/mzgl/app/apple
         lib/mzgl/app/apple/AUv3
         lib/mzgl/app/glfw
         lib/mzgl/app/ios
         lib/mzgl/app/mac
         lib/mzgl/app/mac/GL
         lib/mzgl/app/web
         lib/mzgl/audio
         lib/mzgl/audio/iOS
         lib/mzgl/file
         lib/mzgl/geom
         lib/mzgl/gl
         lib/mzgl/gl/drawing
         lib/mzgl/midi
         lib/mzgl/music
         lib/mzgl/net
         lib/mzgl/ui
         lib/mzgl/ui/widgets
         lib/mzgl/util)
target_include_directories (mzgl PUBLIC lib)

include (cmake/android.cmake)
include (cmake/apple.cmake)
include (cmake/linux.cmake)
include (cmake/windows.cmake)

target_link_libraries (mzgl PUBLIC ${MZGL_LIBRARIES})
