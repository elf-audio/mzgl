function(mzgl_add_portaudio)
  if(ANDROID OR IOS)
    return()
  endif()
  if(DEFINED MZGL_ADDED_PACKAGES)
    return()
  endif()
  set(MZGL_ADDED_PACKAGES
      TRUE
      PARENT_SCOPE)

  mzgl_print_in_yellow("[CPM] -> Adding package gh:PortAudio/portaudio#f217b42")
  mzgl_save_cmake_log_level()

  cmake_policy(SET CMP0077 NEW)
  set(PA_BUILD_STATIC ON)
  set(PA_BUILD_SHARED OFF)

  mzgl_print_in_cyan("CPM PORTAUDIO")
  cpmaddpackage(
    NAME
    PortAudio
    GITHUB_REPOSITORY
    PortAudio/portaudio
    GIT_TAG
    f217b42
    DOWNLOAD_ONLY
    YES
    OPTIONS
    "PA_BUILD_STATIC ON"
    "PA_BUILD_SHARED OFF"
    "PA_USE_JACK OFF")

  mzgl_patch_cmake_version(
    "${${CPM_LAST_PACKAGE_NAME}_SOURCE_DIR}/CMakeLists.txt")

  add_subdirectory("${${CPM_LAST_PACKAGE_NAME}_SOURCE_DIR}"
                   "${CMAKE_BINARY_DIR}/portaudio" EXCLUDE_FROM_ALL)
  include_directories(SYSTEM INTERFACE ${${CPM_LAST_PACKAGE_NAME}_SOURCE_DIR})

  set("${CPM_LAST_PACKAGE_NAME}_DEPS_DIR"
      "${${CPM_LAST_PACKAGE_NAME}_SOURCE_DIR}"
      CACHE INTERNAL "${CPM_LAST_PACKAGE_NAME} Deps dir")
  mzgl_restore_cmake_log_level()
endfunction()

function(mzgl_add_packages)
  mzgl_add_package("gh:nlohmann/json@3.11.3")
  mzgl_add_package("gh:cameron314/concurrentqueue@1.0.4")
  mzgl_add_package("gh:cameron314/readerwriterqueue@1.0.6")
  mzgl_add_package("gh:elf-audio/fontstash#357bef7")
  mzgl_add_package("gh:elf-audio/fast-poly2tri#5dcc516")
endfunction()
