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

# CPMAddPackage("gh:yhirose/cpp-httplib#412ab5f")

# CPMAddPackage( NAME nlohmann_json VERSION 3.9.1 OPTIONS "JSON_BuildTests OFF"
# )

if(CMAKE_BUILD_TYPE MATCHES DEBUG)
  add_compile_definitions(DEBUG=1)

endif()



set(LINUX FALSE)
if(NOT ANDROID
   AND UNIX
   AND NOT APPLE)
  set(LINUX TRUE)
endif()

set(IOS FALSE)
if(DEFINED ${PLATFORM} AND ${PLATFORM} STREQUAL "OS64")
  set(IOS TRUE)
endif()

# PRINT EVERYTHING!!!
set(CMAKE_VERBOSE_MAKEFILE ON)

set(MZGL_SOURCES
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
    lib/mzgl/gl/Image-win.cpp
    lib/mzgl/gl/Image.cpp
    lib/mzgl/gl/Shader.cpp
    lib/mzgl/gl/SvgVbo.cpp
    lib/mzgl/gl/Texture.cpp
    lib/mzgl/gl/Vbo.cpp
    lib/mzgl/gl/ScopedMask.cpp
    lib/mzgl/gl/drawing/Drawer.cpp
    lib/mzgl/gl/drawing/MitredLine.cpp
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
    lib/mzgl/ui/DropDown.cpp
    lib/mzgl/ui/Layer.cpp
    lib/mzgl/ui/Layout.cpp
    lib/mzgl/ui/Scroller.cpp
    lib/mzgl/ui/ScrollingList.cpp
    lib/mzgl/ui/ScrollingListDeletable.cpp
    lib/mzgl/ui/ScrollingListItem.cpp
    lib/mzgl/ui/Slider.cpp
    lib/mzgl/ui/FlexboxLayout.cpp
    lib/mzgl/util/Base64.cpp
    lib/mzgl/util/Base64.h
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
    lib/mzgl/util/mzAssert.cpp
    lib/mzgl/util/stringUtil.cpp
    $<$<PLATFORM_ID:Darwin>:
    lib/mzgl/audio/juce/JuceAudioSystem.cpp
    lib/mzgl/audio/juce/lib/juce_BigInteger.cpp
    lib/mzgl/audio/juce/lib/mzgl_juce_CoreAudio.mm
    lib/mzgl/audio/AudioFileApple.cpp
    lib/mzgl/midi/AllMidiDevicesAppleImpl.mm
    lib/mzgl/midi/appleMidiUtils.cpp
    lib/mzgl/app/mac/MacAppDelegate.mm
    lib/mzgl/app/mac/MacMenuBar.mm
    lib/mzgl/app/mac/EventsView.mm
    lib/mzgl/app/mac/NSBlockButton.m
    lib/mzgl/app/mac/MacFilePickerDelegate.mm
    lib/mzgl/app/mac/GL/MZGLView.mm
    lib/mzgl/app/mac/Metal/MZMetalView.mm
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
if(NOT ANDROID)
  list(
    APPEND
    MZGL_SOURCES
    lib/glew/src/glew.c
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
endif()

if(USING_SOKOL)
  list(
        APPEND
        MZGL_SOURCES
        lib/mzgl/gl/api/Sokol/SokolAPI.cpp
        lib/mzgl/gl/api/Sokol/SokolImplementation.cpp
        lib/mzgl/gl/api/Sokol/SokolVbo.cpp
        lib/mzgl/gl/api/Sokol/SokolShader.cpp
  )
else()
  list(
        APPEND
        MZGL_SOURCES
        lib/mzgl/gl/api/OpenGL/OpenGLAPI.cpp
        lib/mzgl/gl/api/OpenGL/OpenGLShader.cpp
  )
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
    lib/mzgl/app/web
    lib/mzgl/audio
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
    lib/farbot/include
    # lib/ghc-filesystem/include
)

add_subdirectory(lib/ghc-filesystem EXCLUDE_FROM_ALL)
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

add_subdirectory(lib/ZipFile EXCLUDE_FROM_ALL)
# add_dependencies(libmzgl pugixml)

# Apple platforms have a special audio file reader
if(APPLE)

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
    lib/mzgl/gl/api/Sokol/SokolImplementation.cpp

    lib/mzgl/net/FileDownloader.cpp
    lib/mzgl/util/Haptics.cpp
    PROPERTIES COMPILE_FLAGS "-x objective-c++ -fobjc-arc")
else()
  list(APPEND MZGL_SOURCES lib/misc/speex_resampler.cpp)
  list(APPEND MZGL_SOURCES lib/mzgl/audio/Resampler.cpp)
  list(APPEND MZGL_SOURCES lib/mzgl/audio/AudioFileAndroid.cpp)
endif()

if(ANDROID)
  list(APPEND MZGL_SOURCES lib/mzgl/app/android/mainAndroid.cpp
       lib/mzgl/app/android/androidUtil.cpp
       lib/mzgl/midi/AllMidiDevicesAndroidImpl.cpp)
else()
  list(APPEND MZGL_SOURCES lib/mzgl/audio/portaudio/PortAudioSystem.cpp)

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



## COMPILE SOKOL SHADERS
# List of shader files
set(SHADER_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/lib/mzgl/gl/api/Sokol/shaders/nothing.glsl"
    "${CMAKE_CURRENT_SOURCE_DIR}/lib/mzgl/gl/api/Sokol/shaders/color.glsl"
    "${CMAKE_CURRENT_SOURCE_DIR}/lib/mzgl/gl/api/Sokol/shaders/colorTexture.glsl"
    "${CMAKE_CURRENT_SOURCE_DIR}/lib/mzgl/gl/api/Sokol/shaders/tex.glsl"
    "${CMAKE_CURRENT_SOURCE_DIR}/lib/mzgl/gl/api/Sokol/shaders/font.glsl"
)


# Optional: Specify the path if sokol-shdc is not in your PATH
set(SOKOL_SHDC "sokol-shdc")




if(USING_SOKOL)

  # Process each shader file
  foreach (SHADER_SOURCE ${SHADER_SOURCES})
      # Create a corresponding header file name
      get_filename_component(SHADER_NAME ${SHADER_SOURCE} NAME_WE)
      set(SHADER_HEADER "${CMAKE_CURRENT_BINARY_DIR}/${SHADER_NAME}.glsl.h")

      # Custom command that runs sokol-shdc for each shader
      add_custom_command(
              OUTPUT ${SHADER_HEADER}
              COMMAND ${SOKOL_SHDC} --reflection --input ${SHADER_SOURCE} --output ${SHADER_HEADER} --slang glsl300es:hlsl5:metal_macos
              DEPENDS ${SHADER_SOURCE}
              COMMENT "Compiling ${SHADER_SOURCE} to ${SHADER_HEADER}"
              VERBATIM
      )

      # Append the output header to a list of all generated headers
      list(APPEND GENERATED_SHADER_HEADERS ${SHADER_HEADER})
  endforeach ()

  # Custom target that depends on all generated shader headers
  add_custom_target(CompileShaders ALL DEPENDS ${GENERATED_SHADER_HEADERS})

  # Ensure your main application depends on this custom target
  add_dependencies(libmzgl CompileShaders)

  target_include_directories(libmzgl PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

  # uncomment to turn on sokol
  add_compile_definitions(MZGL_SOKOL_METAL)
endif()

if(APPLE)
# choose the sokol backend for platform
add_compile_definitions(SOKOL_METAL)
endif()



set(MZGL_LIBS ZipFile yoga pu_gixml ghc_filesystem)

if(WIN32)
  # list(APPEND MZGL_LIBS zlib) target_include_directories(libmzgl PUBLIC
  # lib/zlib-win)
else()
  list(APPEND MZGL_LIBS z)
endif()

cmake_policy(SET CMP0077 NEW)
set(PA_BUILD_STATIC ON) # CACHE BOOL "build portaudio static library")
set(PA_BUILD_SHARED OFF) # CACHE BOOL "build portaudio shared library")

# set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE
# "lib-windows/release-${OF_PLATFORM}-${ARCH_BIT}")
# set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG
# "lib-windows/debug-${OF_PLATFORM}-${ARCH_BIT}")

if(NOT ANDROID)
  add_subdirectory(lib/glfw "glfw-build" EXCLUDE_FROM_ALL)

  add_library(RtMidi OBJECT lib/rtmidi/RtMidi.cpp)
  # add_subdirectory("lib/RtMidi" "rtmidi-build")
  if(NOT APPLE) # we just link the framework I think
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
  add_compile_definitions(GL_SILENCE_DEPRECATION __MACOSX_CORE__)
  target_link_libraries(
    libmzgl
    "-framework OpenGL -framework Metal -framework MetalKit -framework Accelerate -framework AppKit -framework StoreKit -framework CoreVideo -framework CoreMidi -framework CoreAudio -framework AVFoundation -framework CoreMedia -framework WebKit -framework AudioUnit -framework CoreAudioKit -framework UniformTypeIdentifiers"
  )

elseif(WIN32)

  add_compile_definitions(_WIN32_WINNT=0x0601 GLEW_STATIC)
  add_compile_definitions(__WINDOWS_MM__)

  list(
    APPEND
    MZGL_LIBS
    ${GLFW_LIBRARIES}
    ${OPENGL_gl_LIBRARY}
    winmm
    dsound
    ole32)

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
  add_subdirectory(lib/portaudio "portaudio-build" EXCLUDE_FROM_ALL)
  list(APPEND MZGL_LIBS PortAudio)
endif()
if(WIN32)
  add_subdirectory(lib/portaudio "portaudio-build" EXCLUDE_FROM_ALL)
  list(APPEND MZGL_LIBS PortAudio $<$<PLATFORM_ID:Windows>:Urlmon Wininet>)
endif()

if(NOT ANDROID AND NOT IOS)
  list(APPEND MZGL_LIBS RtMidi glfw)
endif()

target_link_libraries(libmzgl ${MZGL_LIBS})

if(NOT ANDROID)
  target_link_libraries(libmzgl_unit_test ${MZGL_LIBS})
  if(APPLE)
    target_link_libraries(libmzgl_unit_test
                          "-framework CoreVideo -framework WebKit")
  endif()
endif()

if(NOT WIN32)
  add_compile_options(-fdiagnostics-color=always)
endif()

mzgl_supress_warnings()
