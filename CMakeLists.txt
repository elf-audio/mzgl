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
include (cmake/linux.cmake)
include (cmake/properties.cmake)
include (cmake/sources.cmake)

if ("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_CURRENT_SOURCE_DIR}")
  mzgl_download_cpm ()
endif ()
mzgl_add_packages ()

add_subdirectory (lib/elfxml)
add_subdirectory (lib/fsystem)
add_subdirectory (lib/ZipFile)

if (BUILD_PLATFORM_IS_MAC OR BUILD_PLATFORM_IS_LINUX)
  add_subdirectory (lib/glew)
endif ()

add_library (mzgl STATIC ${MZGL_SOURCES})
mzgl_set_target_properties (mzgl)
target_include_directories (mzgl PUBLIC lib lib/mzgl/audio)

add_library (mzgl-plugin STATIC ${MZGL_SOURCES})
mzgl_set_target_properties (mzgl-plugin)
target_include_directories (mzgl-plugin PUBLIC lib lib/mzgl/audio lib/mzgl/app/apple/AUv3)

include (cmake/android.cmake)
include (cmake/apple.cmake)
include (cmake/windows.cmake)

target_link_libraries (mzgl PUBLIC ${MZGL_LIBRARIES})
target_link_libraries (mzgl-plugin PUBLIC ${MZGL_LIBRARIES})

target_compile_definitions (mzgl-plugin PRIVATE MZGL_PLUGIN)

if (BUILD_PLATFORM_IS_ANDROID)
  target_include_directories (mzgl PUBLIC ${ANDROID_NDK}/sources/android/native_app_glue)
endif ()
