function(mzgl_detect_cpm_root_dir)
  if(NOT DEFINED CPM_ROOT_DIR OR "${CPM_ROOT_DIR}" STREQUAL "")
    # Default the CPM cache root to ~/.cpm: shared across build dirs and outside
    # the repo. $ENV{HOME} on Unix/macOS, $ENV{USERPROFILE} on Windows. This is the
    # normal path for the desktop/iOS/macOS presets and scripts/gen-xcode.sh; callers
    # can still override CPM_ROOT_DIR with -D (Android sets it to the repo-relative
    # "cpm-source-cache"). We resolve HOME here in CMake rather than via a preset
    # "$env{HOME}" macro because some IDEs (CLion) mis-expand that macro to a RELATIVE
    # path, which mzgl_cpm_cache_dir() then anchors under CMAKE_SOURCE_DIR - creating
    # the cache inside the repo (e.g. <repo>/Users/<name>/.cpm) instead of in $HOME.
    set(_cpm_home "$ENV{HOME}")
    if(NOT _cpm_home)
      set(_cpm_home "$ENV{USERPROFILE}")
    endif()
    # Must be absolute: a relative/empty home would silently nest the cache in the repo
    # (the relative branch of mzgl_cpm_cache_dir is reserved for the intentional Android
    # "cpm-source-cache"). Fail loudly rather than scatter a bogus ~ dir into the tree.
    if(NOT _cpm_home OR NOT IS_ABSOLUTE "${_cpm_home}")
      message(FATAL_ERROR
        "Cannot locate a home directory for the CPM cache (HOME/USERPROFILE is unset "
        "or not absolute; got '${_cpm_home}'). Pass an absolute path explicitly, e.g. "
        "-DCPM_ROOT_DIR=\"$ENV{HOME}/.cpm\".")
    endif()
    message(STATUS "CPM_ROOT_DIR not set, defaulting to ${_cpm_home}/.cpm")
    set(CPM_ROOT_DIR
        "${_cpm_home}/.cpm"
        CACHE PATH "Path to the CPM root directory")
  endif()
endfunction()

function(mzgl_detect_cpm_sub_dir)
  if(NOT DEFINED CPM_SOURCE_CACHE_SUBDIR OR "${CPM_SOURCE_CACHE_SUBDIR}"
                                            STREQUAL "")
    if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
      message(STATUS "CPM_SOURCE_CACHE_SUBDIR Not set, detected iOS")
      set(CPM_SOURCE_CACHE_SUBDIR
          "ios"
          CACHE PATH "Path to the CPM root directory")

    elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin" OR CMAKE_HOST_SYSTEM_NAME
                                                  STREQUAL "Darwin")
      message(STATUS "CPM_SOURCE_CACHE_SUBDIR Not set, detected macOS")
      set(CPM_SOURCE_CACHE_SUBDIR
          "macOS"
          CACHE PATH "Path to the CPM root directory")
    elseif(ANDROID OR CMAKE_SYSTEM_NAME STREQUAL "Android")
      message(STATUS "CPM_SOURCE_CACHE_SUBDIR Not set, detected Android")
      set(CPM_SOURCE_CACHE_SUBDIR
          "android"
          CACHE PATH "Path to the CPM root directory")
    elseif(WIN32)
      message(STATUS "CPM_SOURCE_CACHE_SUBDIR Not set, detected windows")
      set(CPM_SOURCE_CACHE_SUBDIR
          "windows"
          CACHE PATH "Path to the CPM root directory")
    elseif(UNIX)
      message(
        STATUS
          "CPM_SOURCE_CACHE_SUBDIR Not set, detected unix -> ${CMAKE_SYSTEM_NAME}, ${CMAKE_HOST_SYSTEM_NAME}"
      )
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
