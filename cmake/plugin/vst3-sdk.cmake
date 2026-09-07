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

    include(FetchContent)
    FetchContent_Declare(
      vst3sdk
      GIT_REPOSITORY https://github.com/steinbergmedia/vst3sdk.git
      GIT_TAG v3.7.12_build_20
      GIT_SUBMODULES_RECURSE TRUE
      GIT_SHALLOW FALSE)

    # The SDK's own CMakeLists call cmake_minimum_required() with a version
    # below 3.10, which newer CMake flags as deprecated. We can't edit the
    # fetched sources, so silence deprecation warnings only while the SDK is
    # configured, then restore the previous setting.
    set(_mzgl_vst3_warn_deprecated "${CMAKE_WARN_DEPRECATED}")
    set(CMAKE_WARN_DEPRECATED OFF CACHE BOOL "" FORCE)
    set(CMAKE_POLICY_VERSION_MINIMUM 3.10)

    FetchContent_MakeAvailable(vst3sdk)

    unset(CMAKE_POLICY_VERSION_MINIMUM)
    if(_mzgl_vst3_warn_deprecated STREQUAL "")
      unset(CMAKE_WARN_DEPRECATED CACHE)
    else()
      set(CMAKE_WARN_DEPRECATED "${_mzgl_vst3_warn_deprecated}" CACHE BOOL "" FORCE)
    endif()
  else()
    # Another directory already fetched the SDK; vst3sdk_SOURCE_DIR is scoped
    # to that directory, so recover it from FetchContent's global properties.
    include(FetchContent)
    FetchContent_GetProperties(vst3sdk)
  endif()

  set(VST3_SDK_ROOT ${vst3sdk_SOURCE_DIR})
endmacro()
