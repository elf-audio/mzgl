# Fetches the CLAP headers (header-only, MIT) once per build tree.
# Call mzgl_fetch_clap_sdk() before mzgl_add_clap_plugin(); sets CLAP_SDK_ROOT.
include_guard(GLOBAL)
include(FetchContent)

macro(mzgl_fetch_clap_sdk)
  if(NOT CLAP_SDK_ROOT)
    FetchContent_Declare(
      clapsdk
      GIT_REPOSITORY https://github.com/free-audio/clap.git
      GIT_TAG 1.2.6
      GIT_SHALLOW TRUE)
    # The project's CMakeLists just declares an INTERFACE `clap` target;
    # we only use CLAP_SDK_ROOT/include, so the extra target is harmless.
    FetchContent_MakeAvailable(clapsdk)
    set(CLAP_SDK_ROOT ${clapsdk_SOURCE_DIR})
  endif()
endmacro()
