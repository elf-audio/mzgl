# Fetch + configure the Steinberg VST3 SDK (examples/VSTGUI disabled).
# Include this then call mzgl_fetch_vst3_sdk(). Safe to call from several
# plugin directories - the SDK is only fetched once. Sets VST3_SDK_ROOT in
# the caller's scope.

macro(mzgl_fetch_vst3_sdk)
  if(NOT TARGET sdk)
    set(SMTG_ENABLE_VST3_PLUGIN_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(SMTG_ENABLE_VST3_HOSTING_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(SMTG_ENABLE_VSTGUI_SUPPORT OFF CACHE BOOL "" FORCE)
    set(SMTG_ADD_VSTGUI OFF CACHE BOOL "" FORCE)
    set(SMTG_CREATE_PLUGIN_LINK OFF CACHE BOOL "" FORCE)
    set(SMTG_RUN_VST_VALIDATOR OFF CACHE BOOL "" FORCE)
    set(SMTG_CREATE_MODULE_INFO OFF CACHE BOOL "" FORCE)
    set(SMTG_ENABLE_USE_OF_JACK OFF CACHE BOOL "" FORCE)
    # Match the static CRT (/MT) mzgl's parent projects build with on Windows;
    # the SDK defaults to /MD, which would give LNK2038 runtime mismatches.
    set(SMTG_USE_STATIC_CRT ON CACHE BOOL "" FORCE)

    # Through CPM when it's loaded, so the clone (full history + submodules,
    # slow) lives in the persistent CPM source cache (~/.cpm) and is reused by
    # every build dir and CI run. Plain FetchContent put it in build/_deps,
    # which CI wipes, so it was recloned on every run.
    set(_mzgl_vst3_args
        GIT_REPOSITORY https://github.com/steinbergmedia/vst3sdk.git
        GIT_TAG v3.7.12_build_20
        GIT_SUBMODULES_RECURSE TRUE
        GIT_SHALLOW FALSE)
    if(NOT COMMAND CPMAddPackage)
      include(FetchContent)
      FetchContent_Declare(vst3sdk ${_mzgl_vst3_args})
    endif()

    # The SDK's own CMakeLists call cmake_minimum_required() with a version
    # below 3.10, which newer CMake flags as deprecated. We can't edit the
    # fetched sources, so silence deprecation warnings only while the SDK is
    # configured, then restore the previous setting.
    set(_mzgl_vst3_warn_deprecated "${CMAKE_WARN_DEPRECATED}")
    set(CMAKE_WARN_DEPRECATED OFF CACHE BOOL "" FORCE)
    set(CMAKE_POLICY_VERSION_MINIMUM 3.10)

    if(COMMAND CPMAddPackage)
      CPMAddPackage(NAME vst3sdk ${_mzgl_vst3_args})
    else()
      FetchContent_MakeAvailable(vst3sdk)
    endif()
    set_property(GLOBAL PROPERTY MZGL_VST3_SDK_ROOT "${vst3sdk_SOURCE_DIR}")

    unset(CMAKE_POLICY_VERSION_MINIMUM)
    if(_mzgl_vst3_warn_deprecated STREQUAL "")
      unset(CMAKE_WARN_DEPRECATED CACHE)
    else()
      set(CMAKE_WARN_DEPRECATED "${_mzgl_vst3_warn_deprecated}" CACHE BOOL "" FORCE)
    endif()
  else()
    # Another directory already fetched the SDK; vst3sdk_SOURCE_DIR is scoped
    # to that directory, so recover it from the global property set above.
    get_property(vst3sdk_SOURCE_DIR GLOBAL PROPERTY MZGL_VST3_SDK_ROOT)
  endif()

  set(VST3_SDK_ROOT ${vst3sdk_SOURCE_DIR})
endmacro()
