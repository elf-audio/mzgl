# Fetch Apple's AudioUnitSDK (the modern C++ AUv2 base classes: AUBase,
# AUEffectBase, AUMIDIEffectBase, ...) and build it as a static library target
# `ausdk`. Include this then call mzgl_fetch_auv2_sdk(). Safe to call from
# several plugin directories - fetched/built once. Sets AUV2_SDK_ROOT.
#
# The SDK ships no CMake project (an Xcode project only), so the sources under
# src/AudioUnitSDK are compiled here directly. Pinned to 1.3.0: 1.4 needs C++23.

macro(mzgl_fetch_auv2_sdk)
  if(NOT TARGET ausdk)
    include(FetchContent)
    FetchContent_Declare(
      audiounitsdk
      GIT_REPOSITORY https://github.com/apple/AudioUnitSDK.git
      GIT_TAG AudioUnitSDK-1.3.0
      GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(audiounitsdk)

    file(GLOB _ausdk_sources "${audiounitsdk_SOURCE_DIR}/src/AudioUnitSDK/*.cpp")
    add_library(ausdk STATIC ${_ausdk_sources})
    target_include_directories(ausdk PUBLIC "${audiounitsdk_SOURCE_DIR}/include")
    # AudioUnitSDK 1.3 uses std::span + constraints: C++20 (propagates to consumers).
    target_compile_features(ausdk PUBLIC cxx_std_20)
    # The SDK's -Wall/-Wextra hygiene isn't ours to enforce; keep our build quiet.
    target_compile_options(ausdk PRIVATE -w)
    target_link_libraries(ausdk PUBLIC "-framework AudioToolbox" "-framework AudioUnit"
                                       "-framework CoreAudio" "-framework CoreFoundation")
  else()
    include(FetchContent)
    FetchContent_GetProperties(audiounitsdk)
  endif()

  set(AUV2_SDK_ROOT ${audiounitsdk_SOURCE_DIR})
endmacro()
