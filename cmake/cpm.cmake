function(mzgl_download_cpm)
  if(NOT DEFINED CPM_ROOT_DIR)
    message(FATAL_ERROR "CPM_ROOT_DIR not defined")
  endif()
  if("${CPM_ROOT_DIR}" STREQUAL "")
    message(FATAL_ERROR "CPM_ROOT_DIR is empty")
  endif()
  if(NOT DEFINED CPM_SOURCE_CACHE_SUBDIR)
    message(FATAL_ERROR "CPM_SOURCE_CACHE_SUBDIR not defined")
  endif()
  if("${CPM_SOURCE_CACHE_SUBDIR}" STREQUAL "")
    message(FATAL_ERROR "CPM_SOURCE_CACHE_SUBDIR is empty")
  endif()

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
