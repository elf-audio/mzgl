function(mzgl_detect_target_cpm_dirs)
  if(NOT DEFINED CPM_ROOT_DIR OR "${CPM_ROOT_DIR}" STREQUAL "")
    message(STATUS "CPM_ROOT_DIR Not set, defaulting to cpm-source-cache")
    set(CPM_ROOT_DIR
        "cpm-source-cache"
        CACHE PATH "Path to the CPM root directory")
  endif()

  if(NOT DEFINED CPM_SOURCE_CACHE_SUBDIR OR "${CPM_SOURCE_CACHE_SUBDIR}"
                                            STREQUAL "")
    if(APPLE)
      if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
        message(STATUS "CPM_SOURCE_CACHE_SUBDIR Not set, defaulting to iOS")
        set(CPM_SOURCE_CACHE_SUBDIR
            "iOS"
            CACHE PATH "Path to the CPM root directory")
      else()
        message(STATUS "CPM_SOURCE_CACHE_SUBDIR Not set, defaulting to macOS")
        set(CPM_SOURCE_CACHE_SUBDIR
            "macOS"
            CACHE PATH "Path to the CPM root directory")
      endif()
    elseif(ANDROID)
      message(STATUS "CPM_SOURCE_CACHE_SUBDIR Not set, defaulting to android")
      set(CPM_SOURCE_CACHE_SUBDIR
          "android"
          CACHE PATH "Path to the CPM root directory")
    elseif(WIN32)
      message(STATUS "CPM_SOURCE_CACHE_SUBDIR Not set, defaulting to windows")
      set(CPM_SOURCE_CACHE_SUBDIR
          "windows"
          CACHE PATH "Path to the CPM root directory")
    elseif(UNIX)
      message(STATUS "CPM_SOURCE_CACHE_SUBDIR Not set, defaulting to unix")
      set(CPM_SOURCE_CACHE_SUBDIR
          "unix"
          CACHE PATH "Path to the CPM root directory")
    else()
      message(
        FATAL_ERROR
          "CPM_SOURCE_CACHE_SUBDIR not defined because we dont know what platform this is"
      )
    endif()
  endif()
endfunction()

function(mzgl_download_cpm)
  mzgl_detect_target_cpm_dirs()

  set(CPM_DOWNLOAD_VERSION "0.38.3")
  set(CPM_EXPECTED_HASH
      "cc155ce02e7945e7b8967ddfaff0b050e958a723ef7aad3766d368940cb15494")
  set(CPM_DOWNLOAD_LOCATION
      "${CMAKE_SOURCE_DIR}/${CPM_ROOT_DIR}/${CPM_SOURCE_CACHE_SUBDIR}/cpm/CPM_${CPM_DOWNLOAD_VERSION}.cmake"
  )

  set(ENV{CPM_SOURCE_CACHE}
      "${CMAKE_SOURCE_DIR}/${CPM_ROOT_DIR}/${CPM_SOURCE_CACHE_SUBDIR}")
  set(CPM_SOURCE_CACHE
      "${CMAKE_SOURCE_DIR}/${CPM_ROOT_DIR}/${CPM_SOURCE_CACHE_SUBDIR}"
      CACHE STRING "CPM source dir" FORCE)
  set(XCODE_HEADER_PATH_FILE
      "${CPM_SOURCE_CACHE}/search_paths.xcconfig"
      CACHE STRING "xcode includes" FORCE)

  get_filename_component(CPM_DOWNLOAD_LOCATION ${CPM_DOWNLOAD_LOCATION}
                         ABSOLUTE)

  mzgl_print_verbose_in_cyan(
    "[CPM] --> Downloading CPM.cmake to ${CPM_DOWNLOAD_LOCATION}")

  if(NOT (EXISTS ${CPM_DOWNLOAD_LOCATION}))
    file(
      DOWNLOAD
      https://github.com/cpm-cmake/CPM.cmake/releases/download/v${CPM_DOWNLOAD_VERSION}/CPM.cmake
      ${CPM_DOWNLOAD_LOCATION}
      EXPECTED_HASH SHA256=${CPM_EXPECTED_HASH})

    if(EXISTS "${XCODE_HEADER_PATH_FILE}")
      message(STATUS "File exists: ${XCODE_HEADER_PATH_FILE}. Deleting it.")
      file(REMOVE "${XCODE_HEADER_PATH_FILE}")
    endif()
  else()
    mzgl_print_verbose_in_cyan(
      "[CPM] --> CPM already exists in ${CPM_DOWNLOAD_LOCATION}")
  endif()

  include(${CPM_DOWNLOAD_LOCATION})
endfunction()

function(mzgl_lock_cpm_cache)
  if(CPM_SOURCE_CACHE)
    file(LOCK "${CMAKE_SOURCE_DIR}/${CPM_ROOT_DIR}/${CPM_SOURCE_CACHE_SUBDIR}"
         DIRECTORY GUARD FILE)
  endif()
endfunction()

function(mzgl_unlock_cpm_cache)
  if(CPM_SOURCE_CACHE)
    file(
      LOCK "${CMAKE_SOURCE_DIR}/${CPM_ROOT_DIR}/${CPM_SOURCE_CACHE_SUBDIR}"
      DIRECTORY
      GUARD FILE
      RELEASE)
  endif()
endfunction()
