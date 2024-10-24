function(mzgl_detect_cpm_root_dir)
  if(NOT DEFINED CPM_ROOT_DIR OR "${CPM_ROOT_DIR}" STREQUAL "")
    message(STATUS "CPM_ROOT_DIR Not set, detected cpm-source-cache")
    set(CPM_ROOT_DIR
        "cpm-source-cache"
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
