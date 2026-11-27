# Fetches the CLAP headers (header-only, MIT) once per build tree.
# Call mzgl_fetch_clap_sdk() before mzgl_add_clap_plugin(); sets CLAP_SDK_ROOT.
include_guard(GLOBAL)
include(FetchContent)

macro(mzgl_fetch_clap_sdk)
  if(NOT CLAP_SDK_ROOT)
    # Through CPM when loaded, so the clone lives in the persistent CPM source
    # cache (~/.cpm) instead of build/_deps, which CI wipes every run. Only
    # CLAP_SDK_ROOT/include is used, so the project itself isn't added.
    set(_mzgl_clap_args GIT_REPOSITORY https://github.com/free-audio/clap.git GIT_TAG 1.2.6
                        GIT_SHALLOW TRUE)
    if(COMMAND CPMAddPackage)
      CPMAddPackage(NAME clapsdk ${_mzgl_clap_args} DOWNLOAD_ONLY YES)
    else()
      FetchContent_Declare(clapsdk ${_mzgl_clap_args})
      FetchContent_MakeAvailable(clapsdk)
    endif()
    set(CLAP_SDK_ROOT ${clapsdk_SOURCE_DIR})
  endif()
endmacro()
