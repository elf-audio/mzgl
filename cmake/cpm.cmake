# Resolve the platform-specific CPM cache directory into out_var. CPM_ROOT_DIR may
# be absolute (e.g. the default ~/.cpm) - in which case it is used as-is - or a path
# relative to the project source dir (e.g. "cpm-source-cache").
function(mzgl_cpm_cache_dir out_var)
  if(IS_ABSOLUTE "${CPM_ROOT_DIR}")
    set(${out_var} "${CPM_ROOT_DIR}/${CPM_SOURCE_CACHE_SUBDIR}" PARENT_SCOPE)
  else()
    set(${out_var}
        "${CMAKE_SOURCE_DIR}/${CPM_ROOT_DIR}/${CPM_SOURCE_CACHE_SUBDIR}"
        PARENT_SCOPE)
  endif()
endfunction()

function(mzgl_download_cpm)

  set(CPM_DOWNLOAD_VERSION "0.38.3")
  set(CPM_EXPECTED_HASH
      "cc155ce02e7945e7b8967ddfaff0b050e958a723ef7aad3766d368940cb15494")
  mzgl_cpm_cache_dir(_cpm_cache)
  set(CPM_DOWNLOAD_LOCATION
      "${_cpm_cache}/cpm/CPM_${CPM_DOWNLOAD_VERSION}.cmake"
      CACHE STRING "CPM source dir" FORCE)
  set(ENV{CPM_SOURCE_CACHE} "${_cpm_cache}")
  set(CPM_SOURCE_CACHE
      "${_cpm_cache}"
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
    mzgl_print_debug_in_grey(
      "[CPM] --> CPM already exists in ${CPM_DOWNLOAD_LOCATION}")
  endif()
endfunction()

function(mzgl_lock_cpm_cache)
  if(CPM_SOURCE_CACHE)
    mzgl_cpm_cache_dir(_cpm_cache)
    file(LOCK "${_cpm_cache}" DIRECTORY GUARD FILE)
  endif()
endfunction()

function(mzgl_unlock_cpm_cache)
  if(CPM_SOURCE_CACHE)
    mzgl_cpm_cache_dir(_cpm_cache)
    file(
      LOCK "${_cpm_cache}"
      DIRECTORY
      GUARD FILE
      RELEASE)
  endif()
endfunction()
