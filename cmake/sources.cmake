set (
  MZGL_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/mzgl/App.h
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/mzgl/PluginEditor.h
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/mzgl/animation/Tween.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/mzgl/audio/AudioSystem.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/mzgl/audio/DrAudioFileReader.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/mzgl/geom/3D/Camera.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/mzgl/geom/geomutils.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/mzgl/geom/Rectf.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/mzgl/geom/RoundedRect.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/mzgl/geom/SVG.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/mzgl/geom/Triangulator.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/mzgl/gl/drawing/Drawer.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/mzgl/gl/drawing/MitredLine.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/mzgl/gl/error.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/mzgl/gl/Fbo.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/mzgl/gl/Font.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/mzgl/gl/api/GraphicsAPI.h
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/mzgl/gl/api/OpenGLAPI.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/mzgl/gl/api/OpenGLAPI.h
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/mzgl/gl/api/OpenGLDefaultShaders.h
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/mzgl/gl/Graphics.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/mzgl/gl/Image-win.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/mzgl/gl/Image.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/mzgl/gl/ScopedMask.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/mzgl/gl/Shader.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/mzgl/gl/SvgVbo.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/mzgl/gl/Texture.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/mzgl/gl/Vbo.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/mzgl/midi/AllMidiDevices.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/mzgl/midi/Midi.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/mzgl/midi/MidiMessageParser.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/mzgl/midi/MidiMessageParser.h
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/mzgl/midi/midiMessagePrinting.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/mzgl/midi/midiMessagePrinting.h
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/mzgl/music/scales.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/mzgl/net/netUtil.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/mzgl/ui/DropDown.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/mzgl/ui/FlexboxLayout.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/mzgl/ui/Layer.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/mzgl/ui/Layout.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/mzgl/ui/Scroller.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/mzgl/ui/ScrollingList.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/mzgl/ui/ScrollingListDeletable.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/mzgl/ui/ScrollingListItem.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/mzgl/ui/Slider.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/mzgl/ui/widgets/ReorderableGrid.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/mzgl/util/DateTime.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/mzgl/util/Dialogs.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/mzgl/util/Dylib.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/mzgl/util/errors.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/mzgl/util/FileWatcher.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/mzgl/util/FloatBuffer.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/mzgl/util/Haptics.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/mzgl/util/Int16Buffer.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/mzgl/util/jsonUtil.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/mzgl/util/jsonUtil.h
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/mzgl/util/log.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/mzgl/util/mainThread.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/mzgl/util/maths.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/mzgl/util/mzAssert.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/mzgl/util/ScopedUrl.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/mzgl/util/stringUtil.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/mzgl/util/UndoManager.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/mzgl/util/util.cpp)

if (NOT ANDROID)
  if (NOT BUILD_PLATFORM_IS_IOS)
    list (
      APPEND
      MZGL_SOURCES
      ${CMAKE_CURRENT_SOURCE_DIR}/lib/mzgl/app/glfw/GLFWAppRunner.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/lib/mzgl/app/glfw/DesktopWindowEventHandler.h
      ${CMAKE_CURRENT_SOURCE_DIR}/lib/mzgl/app/glfw/DesktopWindowEventHandler.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/lib/mzgl/app/glfw/DesktopWindowFileDragHandler.h
      ${CMAKE_CURRENT_SOURCE_DIR}/lib/mzgl/app/glfw/GLFWOS.h)
  endif ()

  if (BUILD_PLATFORM_IS_IOS OR BUILD_PLATFORM_IS_MAC)
    list (
      APPEND
      MZGL_SOURCES
      ${CMAKE_CURRENT_SOURCE_DIR}/lib/mzgl/audio/AudioFileApple.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/lib/mzgl/app/apple/AppleWebView.mm
      ${CMAKE_CURRENT_SOURCE_DIR}/lib/mzgl/app/apple/AUv3/AudioUnitViewController.mm
      ${CMAKE_CURRENT_SOURCE_DIR}/lib/mzgl/app/apple/AUv3/MZGLEffectAU.mm
      ${CMAKE_CURRENT_SOURCE_DIR}/lib/mzgl/app/web/MZWebView.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/lib/mzgl/midi/AllMidiDevicesAppleImpl.mm
      ${CMAKE_CURRENT_SOURCE_DIR}/lib/mzgl/midi/appleMidiUtils.cpp)
  endif ()

  if (BUILD_PLATFORM_IS_IOS)
    list (
      APPEND
      MZGL_SOURCES
      ${CMAKE_CURRENT_SOURCE_DIR}/lib/mzgl/app/ios/iOSAppDelegate.h
      ${CMAKE_CURRENT_SOURCE_DIR}/lib/mzgl/app/ios/iOSAppDelegate.mm
      ${CMAKE_CURRENT_SOURCE_DIR}/lib/mzgl/app/ios/mainiOS.mm
      ${CMAKE_CURRENT_SOURCE_DIR}/lib/mzgl/app/ios/MZGLKitView.h
      ${CMAKE_CURRENT_SOURCE_DIR}/lib/mzgl/app/ios/MZGLKitView.mm
      ${CMAKE_CURRENT_SOURCE_DIR}/lib/mzgl/app/ios/MZGLKitViewController.h
      ${CMAKE_CURRENT_SOURCE_DIR}/lib/mzgl/app/ios/MZGLKitViewController.mm
      ${CMAKE_CURRENT_SOURCE_DIR}/lib/mzgl/app/ios/UIBlockButton.h
      ${CMAKE_CURRENT_SOURCE_DIR}/lib/mzgl/app/ios/UIBlockButton.m
      ${CMAKE_CURRENT_SOURCE_DIR}/lib/mzgl/audio/iOS/AudioSystemIOS.h
      ${CMAKE_CURRENT_SOURCE_DIR}/lib/mzgl/audio/iOS/AudioSystemIOS.mm
      ${CMAKE_CURRENT_SOURCE_DIR}/lib/mzgl/midi/appleMidiUtils.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/lib/mzgl/midi/appleMidiUtils.h)
  endif ()

  if (BUILD_PLATFORM_IS_MAC)
    list (
      APPEND
      MZGL_SOURCES
      ${CMAKE_CURRENT_SOURCE_DIR}/lib/mzgl/audio/portaudio/PortAudioSystem.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/lib/mzgl/app/glfw/DesktopWindowFileDragHandler_Mac.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/lib/mzgl/app/glfw/GLFWOS_Mac.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/lib/mzgl/app/mac/mainMac.mm
      ${CMAKE_CURRENT_SOURCE_DIR}/lib/mzgl/app/mac/EventsView.mm
      ${CMAKE_CURRENT_SOURCE_DIR}/lib/mzgl/app/mac/GL/MZGLView.mm
      ${CMAKE_CURRENT_SOURCE_DIR}/lib/mzgl/app/mac/LayerExplorer.mm
      ${CMAKE_CURRENT_SOURCE_DIR}/lib/mzgl/app/mac/MacAppDelegate.mm
      ${CMAKE_CURRENT_SOURCE_DIR}/lib/mzgl/app/mac/MacFilePickerDelegate.mm
      ${CMAKE_CURRENT_SOURCE_DIR}/lib/mzgl/app/mac/MacMenuBar.mm
      ${CMAKE_CURRENT_SOURCE_DIR}/lib/mzgl/app/mac/TextConsole.mm
      ${CMAKE_CURRENT_SOURCE_DIR}/lib/mzgl/app/web/MacWebView.mm
      ${CMAKE_CURRENT_SOURCE_DIR}/lib/mzgl/app/apple/MZGLWebView.mm)
  endif ()

  if (BUILD_PLATFORM_IS_LINUX)
    list (
      APPEND
      MZGL_SOURCES
      ${CMAKE_CURRENT_SOURCE_DIR}/lib/mzgl/audio/portaudio/PortAudioSystem.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/lib/mzgl/app/glfw/DesktopWindowFileDragHandler_Linux.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/lib/mzgl/app/glfw/GLFWOS_Linux.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/lib/mzgl/util/linuxUtil.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/lib/mzgl/app/glfw/mainGlfw.cpp
      lib/speex/speex_resampler.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/lib/mzgl/audio/Resampler.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/lib/mzgl/audio/AudioFileAndroid.cpp)
  endif ()

  if (BUILD_PLATFORM_IS_WINDOWS)
    list (
      APPEND
      MZGL_SOURCES
      ${CMAKE_CURRENT_SOURCE_DIR}/lib/mzgl/audio/portaudio/PortAudioSystem.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/lib/mzgl/app/glfw/DesktopWindowFileDragHandler_Windows.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/lib/mzgl/app/glfw/GLFWOS_Windows.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/lib/mzgl/util/winUtil.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/lib/mzgl/util/AlignedAllocator.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/lib/mzgl/app/glfw/mainGlfw.cpp
      lib/speex/speex_resampler.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/lib/mzgl/audio/Resampler.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/lib/mzgl/audio/AudioFileAndroid.cpp)
  endif ()
endif ()

if (BUILD_PLATFORM_IS_ANDROID)
  list (
    APPEND
    MZGL_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/mzgl/app/android/mainAndroid.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/mzgl/app/android/androidUtil.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/mzgl/midi/AllMidiDevicesAndroidImpl.cpp
    lib/speex/speex_resampler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/mzgl/audio/Resampler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/mzgl/audio/AudioFileAndroid.cpp)
endif ()
