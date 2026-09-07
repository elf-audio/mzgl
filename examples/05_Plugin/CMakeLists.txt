# 05_Plugin - one mzgl ::Plugin + PluginEditor (a stereo gain, src/GainPlugin.h
# and src/GainEditor.h) built as every desktop plug-in format mzgl supports:
#
#   05_PluginVST3       "mzgl Gain.vst3"        any generator; mac/win/linux
#   05_PluginAUv2       "mzgl Gain.component"   any generator; macOS
#   05_PluginAUv3       05_PluginAUv3.appex     Xcode generator only; macOS
#   05_PluginAUv3App    "mzgl Gain.app"         container app embedding the appex
#
# Build (from the examples folder):
#   cmake -Bbuild -GNinja && cmake --build build --target 05_PluginVST3 05_PluginAUv2
#   cmake -Bbuild-xcode -GXcode && cmake --build build-xcode --target 05_PluginAUv3App --config Release
#   05_Plugin/scripts/package.sh            # -> examples/bin/mzgl-gain-<version>.pkg
#
# The VST3 and AUv2 land in examples/bin, ad-hoc signed and ready to copy into
# ~/Library/Audio/Plug-Ins/{VST3,Components} (or `cmake --install build`).
# See README.md.

set(EXAMPLE_NAME "05_Plugin")
project(${EXAMPLE_NAME} CXX C)

set(GAIN_VERSION 0.1.0)
set(GAIN_DIR ${CMAKE_CURRENT_SOURCE_DIR})

include(${GAIN_DIR}/../../cmake/plugin/mzgl-plugin.cmake)

# Shared between all formats.
set(GAIN_SOURCES
    ${GAIN_DIR}/src/GainPlugin.h
    ${GAIN_DIR}/src/GainEditor.h
    ${GAIN_DIR}/src/version.h
    ${GAIN_DIR}/src/entryPoints.cpp)

# --- VST3 --------------------------------------------------------------------
mzgl_fetch_vst3_sdk()
mzgl_add_vst3_plugin(${EXAMPLE_NAME}VST3
  PRODUCT_NAME "mzgl Gain"
  BUNDLE_ID com.elf-audio.mzgl.Gain.VST3
  VERSION ${GAIN_VERSION}
  COPYRIGHT "elf audio"
  SOURCES ${GAIN_SOURCES} ${GAIN_DIR}/src/GainVST3.cpp
  EXTRA_DEFINITIONS GAIN_VERSION="${GAIN_VERSION}")
target_include_directories(${EXAMPLE_NAME}VST3 PRIVATE ${GAIN_DIR}/src)
set_target_properties(${EXAMPLE_NAME}VST3 PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${GAIN_DIR}/../bin)

if(APPLE AND NOT CMAKE_SYSTEM_NAME MATCHES "iOS")
  # --- AUv2 ------------------------------------------------------------------
  # Same AudioComponent identity as the AUv3 (aufx / Gain / Mzgl): install one
  # or the other, not both, or hosts see two "mzgl: Gain" entries.
  mzgl_fetch_auv2_sdk()
  mzgl_add_auv2_plugin(${EXAMPLE_NAME}AUv2
    PRODUCT_NAME "mzgl Gain"
    BUNDLE_ID com.elf-audio.mzgl.Gain.AUv2
    VERSION ${GAIN_VERSION}
    AU_TYPE aufx AU_SUBTYPE Gain AU_MANUFACTURER Mzgl
    AU_NAME "mzgl: Gain"
    AU_DESCRIPTION "mzgl example gain"
    AU_TAGS Effects Dynamics
    FACTORY_FUNCTION GainAUv2Factory
    COPYRIGHT "elf audio"
    SOURCES ${GAIN_SOURCES} ${GAIN_DIR}/src/GainAUv2.cpp
    EXTRA_DEFINITIONS GAIN_VERSION="${GAIN_VERSION}")
  target_include_directories(${EXAMPLE_NAME}AUv2 PRIVATE ${GAIN_DIR}/src)
  set_target_properties(${EXAMPLE_NAME}AUv2 PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${GAIN_DIR}/../bin)

  # --- AUv3 + container app (Xcode generator only; skipped otherwise) --------
  # Pass -DMZGL_DEVELOPMENT_TEAM=<team id> to sign with your Apple Development
  # certificate; without it the appex/app are ad-hoc signed (local testing only).
  mzgl_add_auv3_plugin(${EXAMPLE_NAME}AUv3
    PRODUCT_NAME "mzgl Gain"
    BUNDLE_ID com.elf-audio.mzgl.Gain
    VERSION ${GAIN_VERSION}
    BUILD_NUMBER 1
    AU_TYPE aufx AU_SUBTYPE Gain AU_MANUFACTURER Mzgl
    AU_NAME "mzgl: Gain"
    AU_DESCRIPTION "mzgl example gain"
    AU_TAGS Effects Dynamics
    VIEW_WIDTH 240 VIEW_HEIGHT 360
    DEVELOPMENT_TEAM "${MZGL_DEVELOPMENT_TEAM}"
    COPYRIGHT "elf audio"
    SOURCES ${GAIN_SOURCES}
    EXTRA_DEFINITIONS GAIN_VERSION="${GAIN_VERSION}")
  if(TARGET ${EXAMPLE_NAME}AUv3)
    target_include_directories(${EXAMPLE_NAME}AUv3 PRIVATE ${GAIN_DIR}/src)
  endif()
endif()
