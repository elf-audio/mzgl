# TODO: can make a CPM of nlohmann json, moodycamel stuff TODO: swap out boost
# fs for https://github.com/gulrak/filesystem

set(CMAKE_VERBOSE_MAKEFILE ON)
cmake_minimum_required(VERSION 3.18.1)
project(mzgl)
# makes clang output colours
if(CMAKE_C_COMPILER_ID STREQUAL "Clang" AND CMAKE_CXX_COMPILER_ID STREQUAL
                                            "Clang")
  add_compile_options(-fcolor-diagnostics)
endif()

# if(ANDROID OR WIN32)
set(CMAKE_CXX_STANDARD 17)
# else() set(CMAKE_CXX_STANDARD 14) endif()

set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(cmake/mzgl.cmake)

if(CMAKE_BUILD_TYPE MATCHES DEBUG)
  add_compile_definitions(DEBUG=1)

endif()

set(LINUX FALSE)
if(NOT ANDROID
   AND UNIX
   AND NOT APPLE)
  set(LINUX TRUE)
endif()

# iOS detection. The old check `if(DEFINED ${PLATFORM} AND ${PLATFORM} STREQUAL
# "OS64")` double-expanded ${PLATFORM} (to e.g. `if(DEFINED OS64 AND ...)`), so
# IOS stayed FALSE under the iOS toolchain and desktop-only deps (GLFW, PortAudio,
# RtMidi) were wrongly built for iOS. Detect via the toolchain's CMAKE_SYSTEM_NAME
# or the ios-cmake PLATFORM value (OS64 / OS64COMBINED / SIMULATOR*).
set(IOS FALSE)
if(CMAKE_SYSTEM_NAME STREQUAL "iOS"
   OR PLATFORM MATCHES "^(OS|OS64|OS64COMBINED|SIMULATOR|SIMULATOR64|SIMULATORARM64)$")
  set(IOS TRUE)
endif()

# PRINT EVERYTHING!!!
set(CMAKE_VERBOSE_MAKEFILE ON)

set(MZGL_SOURCES
    lib/mzgl/App.cpp
    lib/mzgl/animation/Tween.cpp
    lib/mzgl/audio/AudioSystem.cpp
    lib/mzgl/geom/Rectf.cpp
    lib/mzgl/geom/RoundedRect.cpp
    lib/mzgl/geom/SVG.cpp
    lib/mzgl/geom/Triangulator.cpp
    lib/mzgl/geom/geomutils.cpp
    lib/mzgl/geom/3D/Camera.cpp
    lib/mzgl/gl/error.cpp
    lib/mzgl/gl/Fbo.cpp
    lib/mzgl/gl/Font.cpp
    lib/mzgl/gl/Graphics.cpp
    lib/mzgl/gl/colors.cpp
    # Image-win.cpp carries the stb_image implementation (used by Image.cpp's
    # loadPngFromData on every platform); it also defines Image::load/save, but
    # those are #if'd out on Apple where Image.mm (ImageIO) provides them.
    lib/mzgl/gl/Image-win.cpp
    lib/mzgl/gl/Image.cpp
    lib/mzgl/gl/Shader.cpp
    lib/mzgl/gl/SvgVbo.cpp
    lib/mzgl/gl/Texture.cpp
    lib/mzgl/gl/Vbo.cpp
    lib/mzgl/gl/ScopedMask.cpp
    lib/mzgl/gl/drawing/Drawer.cpp
    lib/mzgl/gl/drawing/MitredLine.cpp
    lib/mzgl/gl/drawing/ShapeLUT.cpp
    lib/mzgl/midi/Midi.cpp
    lib/mzgl/midi/AllMidiDevices.cpp
    lib/mzgl/midi/MidiMessage.cpp
    lib/mzgl/midi/MidiMessage.h
    lib/mzgl/midi/MidiCCModeDetector.cpp
    lib/mzgl/midi/MidiCCModeDetector.h
    lib/mzgl/midi/MidiCCMessageParser.cpp
    lib/mzgl/midi/MidiCCMessageParser.h
    lib/mzgl/midi/MidiMessageParser.cpp
    lib/mzgl/midi/MidiMessageParser.h
    lib/mzgl/midi/midiMessagePrinting.cpp
    lib/mzgl/music/chords.cpp
    lib/mzgl/music/scales.cpp
    lib/mzgl/net/netUtil.cpp
    lib/mzgl/net/FileDownloader.cpp
    lib/mzgl/net/FileDownloader.h
    lib/mzgl/ui/widgets/ReorderableGrid.cpp
    lib/mzgl/ui/Layer.cpp
    lib/mzgl/ui/Layout.cpp
    lib/mzgl/ui/Scroller.cpp
    lib/mzgl/ui/ScrollingList.cpp
    lib/mzgl/ui/ScrollingListDeletable.cpp
    lib/mzgl/ui/ScrollingListItem.cpp
    lib/mzgl/ui/Slider.cpp
    lib/mzgl/ui/Select.cpp
    lib/mzgl/ui/FlexboxLayout.cpp
    lib/mzgl/util/Base64.cpp
    lib/mzgl/util/Base64.h
    lib/mzgl/util/CrashHandler.cpp
    lib/mzgl/util/CrashHandler.h
    lib/mzgl/util/DateTime.cpp
    lib/mzgl/util/UndoManager.cpp
    lib/mzgl/util/Dialogs.cpp
    lib/mzgl/util/WebViewOverlay.cpp
    lib/mzgl/util/Dylib.cpp
    lib/mzgl/util/errors.cpp
    lib/mzgl/util/FileWatcher.cpp
    lib/mzgl/util/FloatBuffer.cpp
    lib/mzgl/util/Haptics.cpp
    lib/mzgl/util/Int16Buffer.cpp
    lib/mzgl/util/log.cpp
    lib/mzgl/util/maths.cpp
    lib/mzgl/util/ScopedUrl.cpp
    lib/mzgl/util/util.cpp
    lib/mzgl/util/mainThread.cpp
    lib/mzgl/util/TaskRunner.cpp
    lib/mzgl/util/TaskRunner.h
    lib/mzgl/util/mzAssert.cpp
    lib/mzgl/util/pathUtil.h
    lib/mzgl/util/pathUtil.cpp
    lib/mzgl/util/stringUtil.cpp
    $<$<PLATFORM_ID:Darwin>:
    lib/mzgl/audio/juce/JuceAudioSystem.cpp
    lib/mzgl/audio/juce/lib/juce_BigInteger.cpp
    lib/mzgl/audio/juce/lib/mzgl_juce_CoreAudio.mm
    lib/mzgl/audio/AudioFileApple.cpp
    lib/mzgl/audio/AudioFileDrLib.cpp
    lib/mzgl/midi/AllMidiDevicesAppleImpl.mm
    lib/mzgl/midi/appleMidiUtils.cpp
    lib/mzgl/app/mac/MacAppDelegate.mm
    lib/mzgl/app/mac/MacMenuBar.mm
    lib/mzgl/app/mac/EventsView.mm
    lib/mzgl/app/mac/NSBlockButton.m
    lib/mzgl/app/mac/MacFilePickerDelegate.mm
    lib/mzgl/app/mac/GL/MZGLView.mm
    lib/mzgl/app/mac/LayerExplorer.mm
    lib/mzgl/app/mac/TextConsole.mm
    lib/mzgl/app/mac/mainMac.mm
    lib/mzgl/app/apple/MZGLWebView.mm
    lib/mzgl/app/apple/AppleWebView.mm
    lib/mzgl/app/apple/AUv3/AudioUnitViewController.mm
    lib/mzgl/app/apple/AUv3/MZGLEffectAU.mm
    lib/mzgl/app/web/MacWebView.mm
    lib/mzgl/app/web/MZWebView.cpp
    >)

# iOS-only app/audio sources. The Darwin block above is shared with macOS and
# provides the apple-common pieces (CoreMIDI, AudioFileApple, WKWebView, AUv3);
# the mac-only files in it self-guard out on iOS. These are the UIKit/CoreAudio
# implementations mzgl needs to actually run on iOS, which previously only the
# hand-tooled mzgl.xcodeproj built.
if(IOS)
  list(APPEND MZGL_SOURCES
       lib/mzgl/audio/iOS/AudioSystemIOS.mm
       lib/mzgl/app/ios/mainiOS.mm
       lib/mzgl/app/ios/iOSAppDelegate.mm
       lib/mzgl/app/ios/MZGLKitView.mm
       lib/mzgl/app/ios/MZGLKitViewController.mm
       lib/mzgl/app/ios/TextboxSegmentedViewController.m
       lib/mzgl/app/ios/UIBlockButton.m
       lib/AudioShare/AudioShareSDK.m
       # Apple-shared sources from the $<PLATFORM_ID:Darwin> block above (which is
       # FALSE on iOS); the mac-only files in that block are correctly omitted.
       lib/mzgl/audio/AudioFileApple.cpp
       # dr_libs-based loader (loadViaDrLib/loadResampledViaDrLib): cross-platform,
       # in the Darwin block for macOS and the non-Apple branch for desktop, but the
       # iOS block must add it explicitly (the unit tests exercise it on iOS too).
       lib/mzgl/audio/AudioFileDrLib.cpp
       lib/mzgl/midi/AllMidiDevicesAppleImpl.mm
       lib/mzgl/midi/appleMidiUtils.cpp
       # MZGLWebView.mm / app/web/MacWebView.mm are AppKit (macOS-only); iOS uses
       # AppleWebView (UIKit) on top of the cross-platform MZWebView base.
       lib/mzgl/app/apple/AppleWebView.mm
       lib/mzgl/app/apple/AUv3/AudioUnitViewController.mm
       lib/mzgl/app/apple/AUv3/MZGLEffectAU.mm
       lib/mzgl/app/web/MZWebView.cpp)
endif()

if(NOT ANDROID AND NOT IOS)
  # GLFW desktop app-runner + glew. Not used on iOS (which runs via app/ios/
  # mainiOS.mm); glfw3.h pulls in OpenGL/gl.h which doesn't exist on iOS.
  list(
    APPEND
    MZGL_SOURCES
    lib/mzgl/app/glfw/GLFWAppRunner.cpp
    lib/mzgl/app/glfw/DesktopWindowEventHandler.h
    lib/mzgl/app/glfw/DesktopWindowEventHandler.cpp
    lib/mzgl/app/glfw/DesktopWindowFileDragHandler.h
    lib/mzgl/app/glfw/GLFWOS.h
    $<$<PLATFORM_ID:Linux>:
    lib/mzgl/app/glfw/DesktopWindowFileDragHandler_Linux.cpp
    lib/mzgl/app/glfw/GLFWOS_Linux.cpp
    >
    $<$<PLATFORM_ID:Windows>:
    lib/mzgl/app/glfw/DesktopWindowFileDragHandler_Windows.cpp
    lib/mzgl/app/glfw/GLFWOS_Windows.cpp
    >
    $<$<PLATFORM_ID:Darwin>:
    lib/mzgl/app/glfw/DesktopWindowFileDragHandler_Mac.cpp
    lib/mzgl/app/glfw/GLFWOS_Mac.cpp
    >)
  if(NOT USING_SOKOL OR NOT WIN32)
    list(APPEND MZGL_SOURCES lib/glew/src/glew.c)
  endif()
endif()

if(USING_SOKOL)
  list(
    APPEND
    MZGL_SOURCES
    lib/mzgl/gl/api/sokol/SokolAPI.cpp
    lib/mzgl/gl/api/sokol/SokolImplementation.cpp
    lib/mzgl/gl/api/sokol/SokolVbo.cpp
    lib/mzgl/gl/api/sokol/SokolShader.cpp
    $<$<PLATFORM_ID:Darwin>:
    lib/mzgl/app/mac/Metal/MZMetalView.mm
    >
    $<$<PLATFORM_ID:Windows>:
    lib/mzgl/app/win64/D3D11Context.cpp
    >)
else()
  list(APPEND MZGL_SOURCES lib/mzgl/gl/api/OpenGL/OpenGLAPI.cpp
       lib/mzgl/gl/api/OpenGL/OpenGLShader.cpp)
endif()

foreach(source IN LISTS MZGL_SOURCES)
  get_filename_component(source_path "${source}" PATH)
  string(REPLACE "/" "\\" source_path_group "${source_path}")
  source_group("${source_path_group}" FILES "${source}")
endforeach()

set(HEADERS
    lib/mzgl
    lib/mzgl/animation
    lib/mzgl/app/android
    lib/mzgl/app/apple
    lib/mzgl/app/apple/AUv3
    lib/mzgl/app/mac
    lib/mzgl/app/mac/GL
    lib/mzgl/app/mac/Metal
    lib/mzgl/app/glfw
    lib/mzgl/app/win64
    lib/mzgl/app/web
    lib/mzgl/audio
    lib/mzgl/audio/mac
    lib/mzgl/audio/portaudio
    lib/mzgl/audio/juce
    lib/mzgl/audio/juce/lib
    lib/mzgl/audio/iOS
    lib/mzgl/file
    lib/mzgl/geom
    lib/mzgl/gl
    lib/mzgl/gl/api
    lib/mzgl/gl/api/OpenGL
    lib/mzgl/gl/api/sokol
    lib/mzgl/gl/drawing
    lib/mzgl/midi
    lib/mzgl/music
    lib/mzgl/net
    lib/mzgl/ui
    lib/mzgl/ui/widgets
    lib/mzgl/util
    lib/glm
    lib
    lib/glfw/include/GLFW
    lib/glew/include/GL
    lib/glew/include
    lib/portaudio/include
    lib/fontstash
    lib/rtmidi
    lib/sokol
    lib/pu_gixml/src
    lib/misc
    lib/yoga
    # lib/ghc-filesystem/include
)

# include_directories() list(APPEND HEADERS "${filesystem_SOURCE_DIR}/include")
set(YOGA_SOURCES
    lib/yoga/Utils.cpp
    lib/yoga/YGConfig.cpp
    lib/yoga/YGEnums.cpp
    lib/yoga/YGLayout.cpp
    lib/yoga/YGNode.cpp
    lib/yoga/YGNodePrint.cpp
    lib/yoga/YGStyle.cpp
    lib/yoga/YGValue.cpp
    lib/yoga/Yoga.cpp
    lib/yoga/ylog.cpp
    lib/yoga/event/event.cpp)

# need to move to ZipFile CMakeLists if(WIN32) list(APPEND ZIPPER_SOURCES
# lib/zipper/minizip/iowin32.c) list(APPEND HEADERS lib/zipper/zipper/tps)
# endif()

option(
  MZGL_UNIT_TEST
  "should this be a unit testing build - omits a main, so catch2 can take control"
  OFF)

if(MZGL_UNIT_TEST)
  add_compile_definitions(UNIT_TEST DEBUG=1)
  add_compile_options(-g)
endif(MZGL_UNIT_TEST)

# add_library(zipper STATIC ${ZIPPER_SOURCES}) add_library(boostfs STATIC
# ${BOOST_SOURCES})

add_library(yoga STATIC ${YOGA_SOURCES})
add_library(pu_gixml OBJECT lib/pu_gixml/src/pu_gixml.cpp)

mzgl_print_in_blue("[SUBMODULE] -> Adding submodule lib/ZipFile")
mzgl_save_cmake_log_level()
add_subdirectory(lib/ZipFile EXCLUDE_FROM_ALL)
mzgl_restore_cmake_log_level()
# add_dependencies(libmzgl pugixml)

# Apple platforms have a special audio file reader
if(APPLE)

  # Image::load/save use ImageIO/CoreGraphics on Apple (Image.mm), matching the
  # hand-tooled Xcode projects; every other platform uses the stb_image fallback
  # (Image-win.cpp, added in the else branch below). Image.mm is compiled with ARC
  # (it creates autoreleased NSString/NSURL); on iOS the target is ARC-wide already,
  # the per-file flag makes macOS match.
  list(APPEND MZGL_SOURCES lib/mzgl/gl/Image.mm)
  set_source_files_properties(lib/mzgl/gl/Image.mm
                              PROPERTIES COMPILE_FLAGS "-fobjc-arc")

  # force ARC - we were doing this but it breaks GLFW at the moment because it
  # isn't ARC compatible yet, so we're just selectively compiling with arc for
  # MZGLEffectAU below instead. add_compile_options(-fobjc-arc)

  # these are the files that have objective-c in them
  set_source_files_properties(lib/mzgl/app/apple/AUv3/MZGLEffectAU.mm
                              PROPERTIES COMPILE_FLAGS "-fobjc-arc")

  # these are the files that have objective-c in them
  set_source_files_properties(
    lib/mzgl/App.cpp
    lib/mzgl/util/Dialogs.cpp
    lib/mzgl/util/util.cpp
    lib/mzgl/audio/AudioFileApple.cpp
    lib/mzgl/util/log.cpp
    lib/mzgl/util/WebViewOverlay.cpp
    lib/mzgl/net/netUtil.cpp
    lib/mzgl/gl/api/sokol/SokolImplementation.cpp
    lib/mzgl/net/FileDownloader.cpp
    lib/mzgl/util/Haptics.cpp
    PROPERTIES COMPILE_FLAGS "-x objective-c++ -fobjc-arc")
else()
  list(APPEND MZGL_SOURCES lib/misc/speex_resampler.cpp)
  list(APPEND MZGL_SOURCES lib/mzgl/audio/Resampler.cpp)
  list(APPEND MZGL_SOURCES lib/mzgl/audio/AudioFileAndroid.cpp)
  list(APPEND MZGL_SOURCES lib/mzgl/audio/AudioFileDrLib.cpp)
endif()

if(ANDROID)
  list(APPEND MZGL_SOURCES lib/mzgl/app/android/mainAndroid.cpp
       lib/mzgl/app/android/androidUtil.cpp
       lib/mzgl/midi/AllMidiDevicesAndroidImpl.cpp)
else()
  # iOS uses AudioSystemIOS (CoreAudio via AVAudioSession), not PortAudio. Building
  # PortAudioSystem.cpp on iOS both pulls in the (absent) PortAudio lib and makes
  # its `typedef PortAudioSystem AudioSystem` collide with AudioSystemIOS's.
  if(NOT IOS)
    list(APPEND MZGL_SOURCES lib/mzgl/audio/portaudio/PortAudioSystem.cpp)
  endif()

  if(APPLE)
    if(NOT IOS)
      list(APPEND MZGL_SOURCES lib/mzgl/audio/mac/CoreAudioSystem.cpp
           lib/mzgl/audio/mac/CoreAudioSystem.h)
    endif()
  endif()

  if(NOT APPLE)
    list(APPEND MZGL_SOURCES lib/mzgl/app/glfw/mainGlfw.cpp)
  endif()

endif()

if(WIN32)

  list(APPEND MZGL_SOURCES
       # lib/mzgl/app/win64/mainWin.cpp
       lib/mzgl/util/winUtil.cpp lib/mzgl/util/AlignedAllocator.cpp)
  # moved to ZipFile CMakeLists list(APPEND HEADERS lib/zipper/zipper/tps)

endif()

if(LINUX)
  list(APPEND MZGL_SOURCES
       # lib/mzgl/app/linux/mainLinux.cpp
       lib/mzgl/util/linuxUtil.cpp)
endif()

# iOS-only header search paths: the app/ios UIKit views (included from e.g.
# util/Dialogs.cpp) and the AudioShare SDK (included from app/ios/iOSAppDelegate.mm).
# The hand-tooled project exposed AudioShare via $(MZGLROOT)/lib/AudioShare.
if(IOS)
  list(APPEND HEADERS lib/AudioShare lib/mzgl/app/ios)
endif()

add_library(
  libmzgl STATIC ${MZGL_SOURCES} lib/mzgl/midi/midiMessagePrinting.cpp
                 lib/mzgl/midi/midiMessagePrinting.h)
add_library(libmzgl_unit_test STATIC ${MZGL_SOURCES})
target_include_directories(libmzgl PUBLIC ${HEADERS})
target_include_directories(libmzgl_unit_test PUBLIC ${HEADERS})
target_include_directories(libmzgl PUBLIC ${HEADERS})
target_include_directories(libmzgl_unit_test PUBLIC ${HEADERS})
include_directories(libmzgl PUBLIC ${HEADERS})
include_directories(libmzgl_unit_test PUBLIC ${HEADERS})
target_compile_definitions(libmzgl_unit_test PUBLIC -DUNIT_TEST -DDEBUG)

# COMPILE SOKOL SHADERS List of shader files
set(SHADER_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/lib/mzgl/gl/api/sokol/shaders/nothing.glsl"
    "${CMAKE_CURRENT_SOURCE_DIR}/lib/mzgl/gl/api/sokol/shaders/color.glsl"
    "${CMAKE_CURRENT_SOURCE_DIR}/lib/mzgl/gl/api/sokol/shaders/colorTexture.glsl"
    "${CMAKE_CURRENT_SOURCE_DIR}/lib/mzgl/gl/api/sokol/shaders/tex.glsl"
    "${CMAKE_CURRENT_SOURCE_DIR}/lib/mzgl/gl/api/sokol/shaders/font.glsl"
    "${CMAKE_CURRENT_SOURCE_DIR}/lib/mzgl/gl/api/sokol/shaders/colorFont.glsl")

# Optional: Specify the path if sokol-shdc is not in your PATH
set(SOKOL_SHDC "sokol-shdc")

if(USING_SOKOL)

  # MZGL_USE_PRECOMPILED_SOKOL_SHADERS lets parent projects skip shader
  # regeneration entirely and use the checked-in xcodeShaders/ headers. Useful
  # when only a newer sokol-shdc is available, since the existing mzgl shaders
  # predate the layout(binding=X) requirement.
  if(MZGL_USE_PRECOMPILED_SOKOL_SHADERS)
    set(SOKOL_SHDC_FOUND
        OFF
        CACHE INTERNAL "" FORCE)
  else()
    # Also search the common user-local install dir; CI runners often don't have
    # ~/.local/bin on PATH even when sokol-shdc is installed for the user.
    find_program(SOKOL_SHDC_FOUND
      NAMES ${SOKOL_SHDC}
      HINTS "$ENV{USERPROFILE}/.local/bin" "$ENV{HOME}/.local/bin")
  endif()
  if(SOKOL_SHDC_FOUND)
    # Process each shader file
    foreach(SHADER_SOURCE ${SHADER_SOURCES})
      # Create a corresponding header file name
      get_filename_component(SHADER_NAME ${SHADER_SOURCE} NAME_WE)
      set(SHADER_HEADER "${CMAKE_CURRENT_BINARY_DIR}/${SHADER_NAME}.glsl.h")

      # Custom command that runs sokol-shdc for each shader. Use the full path
      # resolved by find_program (SOKOL_SHDC_FOUND) rather than the bare name, so
      # the build doesn't depend on sokol-shdc being on PATH at build time.
      add_custom_command(
        OUTPUT ${SHADER_HEADER}
        COMMAND ${SOKOL_SHDC_FOUND} --reflection --input ${SHADER_SOURCE} --output
                ${SHADER_HEADER} --slang glsl300es:hlsl5:metal_macos:metal_ios
        DEPENDS ${SHADER_SOURCE}
        COMMENT "Compiling ${SHADER_SOURCE} to ${SHADER_HEADER}"
        VERBATIM)

      # Append the output header to a list of all generated headers
      list(APPEND GENERATED_SHADER_HEADERS ${SHADER_HEADER})
    endforeach()

    # Custom target that depends on all generated shader headers
    add_custom_target(CompileShaders ALL DEPENDS ${GENERATED_SHADER_HEADERS})

    # Ensure your main application depends on this custom target
    add_dependencies(libmzgl CompileShaders)

    target_include_directories(libmzgl PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
    # libmzgl_unit_test compiles the same sokol sources (SokolAPI.cpp ->
    # SokolDefaultShaders.h -> *.glsl.h), so it needs the generated headers too.
    if(TARGET libmzgl_unit_test)
      add_dependencies(libmzgl_unit_test CompileShaders)
      target_include_directories(libmzgl_unit_test PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
    endif()
  else()
    message(STATUS "sokol-shdc not found, using pre-compiled shader headers")
    target_include_directories(
      libmzgl
      PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/lib/mzgl/gl/api/sokol/xcodeShaders")
    # Same precompiled headers for the unit-test build of mzgl.
    if(TARGET libmzgl_unit_test)
      target_include_directories(
        libmzgl_unit_test
        PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/lib/mzgl/gl/api/sokol/xcodeShaders")
    endif()
  endif()

  add_compile_definitions(MZGL_SOKOL)
  if(APPLE)
    add_compile_definitions(SOKOL_METAL)
  elseif(WIN32)
    add_compile_definitions(SOKOL_D3D11)
  endif()
endif()

set(MZGL_LIBS ZipFile yoga pu_gixml concurrentqueue readerwriterqueue)

if(WIN32)
  # list(APPEND MZGL_LIBS zlib) target_include_directories(libmzgl PUBLIC
  # lib/zlib-win)
else()
  list(APPEND MZGL_LIBS z)
endif()

cmake_policy(SET CMP0077 NEW)
set(PA_BUILD_STATIC ON)
set(PA_BUILD_SHARED OFF)

if(NOT ANDROID)
  mzgl_print_in_blue("[SUBMODULE] -> Adding submodule lib/glfw")
  mzgl_save_cmake_log_level()
  add_subdirectory(lib/glfw "glfw-build" EXCLUDE_FROM_ALL)
  mzgl_restore_cmake_log_level()

  add_library(RtMidi OBJECT lib/rtmidi/RtMidi.cpp)
  # add_subdirectory("lib/RtMidi" "rtmidi-build")
  if(APPLE)
    set_source_files_properties(lib/rtmidi/RtMidi.cpp
                                PROPERTIES COMPILE_FLAGS "-Wno-vla-extension")
  elseif(NOT USING_SOKOL)
    find_package(OpenGL REQUIRED)
  endif()
  add_dependencies(libmzgl RtMidi)
endif()

if(ANDROID)

  include_directories(${ANDROID_NDK}/sources/android/native_app_glue/)

  # is this line needed??
  set(${CMAKE_C_FLAGS}, "${CMAKE_C_FLAGS}")

  add_library(
    native_app_glue STATIC
    ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c)

  # find_package (oboe REQUIRED CONFIG)

  set(OBOE_DIR ../../android/oboe)
  add_subdirectory(${OBOE_DIR} ./oboe)
  include_directories(${OBOE_DIR}/include)

  # -DAUTO_TEST
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -O3 -fno-stack-protector")

  # Export ANativeActivity_onCreate(), Refer to:
  # https://github.com/android-ndk/ndk/issues/381.
  set(CMAKE_SHARED_LINKER_FLAGS
      "${CMAKE_SHARED_LINKER_FLAGS} -u ANativeActivity_onCreate")
  list(
    APPEND
    MZGL_LIBS
    # android specific
    android
    native_app_glue
    EGL
    # oboe::oboe
    oboe
    GLESv3
    log
    mediandk)

elseif(APPLE)
  if(IOS)
    # iOS frameworks: no OpenGL (uses Metal via sokol), no AppKit/IOKit/AudioUnit
    # (macOS-only); UIKit replaces AppKit.
    add_compile_definitions(GLES_SILENCE_DEPRECATION GL_SILENCE_DEPRECATION)
    target_link_libraries(
      libmzgl
      "-framework UIKit"
      "-framework GLKit"
      "-framework Metal"
      "-framework MetalKit"
      "-framework Accelerate"
      "-framework StoreKit"
      "-framework CoreVideo"
      "-framework CoreMidi"
      "-framework CoreAudio"
      "-framework AVFoundation"
      "-framework CoreMedia"
      "-framework WebKit"
      "-framework CoreAudioKit"
      "-framework UniformTypeIdentifiers"
      "-framework AudioToolbox"
      "-framework QuartzCore"
      "-framework CoreGraphics"
      "-framework ImageIO"
      "-framework Foundation")
  else()
    add_compile_definitions(GL_SILENCE_DEPRECATION __MACOSX_CORE__)
    target_link_libraries(
      libmzgl
      "-framework OpenGL"
      "-framework Metal"
      "-framework MetalKit"
      "-framework Accelerate"
      "-framework AppKit"
      "-framework StoreKit"
      "-framework CoreVideo"
      "-framework CoreMidi"
      "-framework CoreAudio"
      "-framework AVFoundation"
      "-framework CoreMedia"
      "-framework WebKit"
      "-framework AudioUnit"
      "-framework CoreAudioKit"
      "-framework UniformTypeIdentifiers"
      "-framework AudioToolbox"
      # ImageIO + CoreGraphics back Image::load/save (Image.mm) on macOS.
      "-framework ImageIO"
      "-framework CoreGraphics"
      "-framework IOKit")
  endif()

elseif(WIN32)

  add_compile_definitions(_WIN32_WINNT=0x0601 __WINDOWS_MM__)
  if(NOT USING_SOKOL)
    add_compile_definitions(GLEW_STATIC)
  endif()

  list(
    APPEND
    MZGL_LIBS
    ${GLFW_LIBRARIES}
    winmm
    dsound
    ole32
    # Media Foundation - used for AAC/m4a/wma decoding via IMFSourceReader
    mfplat
    mfreadwrite
    mfuuid)

  if(USING_SOKOL)
    list(APPEND MZGL_LIBS d3d11 dxgi dxguid)
  else()
    list(APPEND MZGL_LIBS ${OPENGL_gl_LIBRARY})
  endif()

elseif(LINUX)
  add_compile_definitions(__LINUX_ALSA__)
  find_package(ALSA REQUIRED)

  # -mfpu=neon-vfpv4
  if(__ARMEL__)
    add_compile_options(-mcpu=cortex-a7 -mfloat-abi=hard -mfpu=neon -O3)

  else(__ARMEL__)
    # add_compile_options(-O3) add_compile_options(-g -O0)
  endif(__ARMEL__)

  # probably want pulseaudio because the latency is supposedly better
  # add_compile_definitions(__LINUX_PULSE__)

  find_package(PkgConfig REQUIRED)
  pkg_check_modules(GTK REQUIRED gtk+-3.0)

  # Setup CMake to use GTK+, tell the compiler where to look for headers and to
  # the linker where to look for libraries
  include_directories(${GTK_INCLUDE_DIRS})
  link_directories(${GTK_LIBRARY_DIRS})

  # Add other flags to the compiler
  add_definitions(${GTK_CFLAGS_OTHER})

  # Check for Jack (any OS)
  find_library(JACK_LIB jack)
  find_package(PkgConfig)
  pkg_check_modules(jack jack)
  if(JACK_LIB OR jack_FOUND)
    set(HAVE_JACK TRUE)
  endif()
  if(NOT HAVE_JACK)
    message(FATAL_ERROR "Jack API requested but no Jack dev libraries found")
  endif()
  if(NOT jack_FOUND)
    list(APPEND MZGL_LIBS ${JACK_LIB})
  else()
    list(APPEND MZGL_LIBS ${jack_LINK_LIBRARIES})
  endif()

  list(
    APPEND
    MZGL_LIBS
    atomic # needed for raspberry pi
    GLESv2
    EGL
    ${GTK_LIBRARIES}
    ${ALSA_LIBRARIES})
endif()

if(NOT ANDROID
   AND NOT WIN32
   AND NOT IOS)
  mzgl_print_in_blue("[SUBMODULE] -> Adding submodule lib/PortAudio")
  mzgl_save_cmake_log_level()
  # PortAudio's CMakeLists declares cmake_minimum_required() below 3.10, which
  # CMake flags as a deprecation warning. Silence it only while PortAudio is
  # configured, then restore the previous setting.
  set(_mzgl_pa_warn_deprecated "${CMAKE_WARN_DEPRECATED}")
  set(CMAKE_WARN_DEPRECATED
      OFF
      CACHE BOOL "" FORCE)
  set(CMAKE_POLICY_VERSION_MINIMUM 3.10)
  add_subdirectory(lib/portaudio "portaudio-build" EXCLUDE_FROM_ALL)
  unset(CMAKE_POLICY_VERSION_MINIMUM)
  if(_mzgl_pa_warn_deprecated STREQUAL "")
    unset(CMAKE_WARN_DEPRECATED CACHE)
  else()
    set(CMAKE_WARN_DEPRECATED
        "${_mzgl_pa_warn_deprecated}"
        CACHE BOOL "" FORCE)
  endif()
  list(APPEND MZGL_LIBS PortAudio)
  mzgl_restore_cmake_log_level()
endif()
if(WIN32)
  mzgl_print_in_blue("[SUBMODULE] -> Adding submodule lib/PortAudio")
  mzgl_save_cmake_log_level()
  set(PA_USE_ASIO
      ON
      CACHE BOOL "Enable PortAudio ASIO support" FORCE)
  # See note above: silence PortAudio's pre-3.10 cmake_minimum_required warning.
  set(_mzgl_pa_warn_deprecated "${CMAKE_WARN_DEPRECATED}")
  set(CMAKE_WARN_DEPRECATED
      OFF
      CACHE BOOL "" FORCE)
  set(CMAKE_POLICY_VERSION_MINIMUM 3.10)
  add_subdirectory(lib/portaudio "portaudio-build" EXCLUDE_FROM_ALL)
  unset(CMAKE_POLICY_VERSION_MINIMUM)
  if(_mzgl_pa_warn_deprecated STREQUAL "")
    unset(CMAKE_WARN_DEPRECATED CACHE)
  else()
    set(CMAKE_WARN_DEPRECATED
        "${_mzgl_pa_warn_deprecated}"
        CACHE BOOL "" FORCE)
  endif()
  list(APPEND MZGL_LIBS PortAudio $<$<PLATFORM_ID:Windows>:Urlmon Wininet>)
  mzgl_restore_cmake_log_level()
endif()
if(NOT ANDROID AND NOT IOS)
  list(APPEND MZGL_LIBS RtMidi glfw)
endif()

target_link_libraries(libmzgl ${MZGL_LIBS})

if(NOT ANDROID)
  target_link_libraries(libmzgl_unit_test ${MZGL_LIBS})
  if(APPLE AND NOT IOS)
    # Mirror the Apple frameworks linked into libmzgl above. Targets that link
    # libmzgl_unit_test (the tests and koala-headless binaries) need the same
    # set - in particular Metal/MetalKit, since the sokol/Metal backend compiles
    # MZMetalView (MTKView, MTLCreateSystemDefaultDevice) into this lib. Without
    # them the macOS test/headless link fails with undefined Metal symbols.
    target_link_libraries(
      libmzgl_unit_test
      "-framework OpenGL"
      "-framework Metal"
      "-framework MetalKit"
      "-framework Accelerate"
      "-framework AppKit"
      "-framework StoreKit"
      "-framework CoreVideo"
      "-framework CoreMidi"
      "-framework CoreAudio"
      "-framework AVFoundation"
      "-framework CoreMedia"
      "-framework WebKit"
      "-framework AudioUnit"
      "-framework CoreAudioKit"
      "-framework UniformTypeIdentifiers"
      "-framework AudioToolbox"
      # ImageIO + CoreGraphics back Image::load/save (Image.mm) on macOS.
      "-framework ImageIO"
      "-framework CoreGraphics"
      "-framework IOKit")
  endif()
endif()

if(NOT WIN32)
  add_compile_options(-fdiagnostics-color=always)
endif()

mzgl_supress_warnings()
