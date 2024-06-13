set (
  MZGL_SOURCES
  lib/mzgl/App.h
  lib/mzgl/PluginEditor.h
  lib/mzgl/animation/Tween.cpp
  lib/mzgl/audio/AudioSystem.cpp
  lib/mzgl/audio/DrAudioFileReader.cpp
  lib/mzgl/geom/3D/Camera.cpp
  lib/mzgl/geom/geomutils.cpp
  lib/mzgl/geom/Rectf.cpp
  lib/mzgl/geom/RoundedRect.cpp
  lib/mzgl/geom/SVG.cpp
  lib/mzgl/geom/Triangulator.cpp
  lib/mzgl/gl/drawing/Drawer.cpp
  lib/mzgl/gl/drawing/MitredLine.cpp
  lib/mzgl/gl/error.cpp
  lib/mzgl/gl/Fbo.cpp
  lib/mzgl/gl/Font.cpp
  lib/mzgl/gl/api/GraphicsAPI.h
  lib/mzgl/gl/api/OpenGLAPI.cpp
  lib/mzgl/gl/api/OpenGLAPI.h
  lib/mzgl/gl/api/OpenGLDefaultShaders.h
  lib/mzgl/gl/Graphics.cpp
  lib/mzgl/gl/Image-win.cpp
  lib/mzgl/gl/Image.cpp
  lib/mzgl/gl/ScopedMask.cpp
  lib/mzgl/gl/Shader.cpp
  lib/mzgl/gl/SvgVbo.cpp
  lib/mzgl/gl/Texture.cpp
  lib/mzgl/gl/Vbo.cpp
  lib/mzgl/midi/AllMidiDevices.cpp
  lib/mzgl/midi/Midi.cpp
  lib/mzgl/midi/midiMessageParser.cpp
  lib/mzgl/midi/midiMessageParser.h
  lib/mzgl/midi/midiMessagePrinting.cpp
  lib/mzgl/midi/midiMessagePrinting.h
  lib/mzgl/music/scales.cpp
  lib/mzgl/net/netUtil.cpp
  lib/mzgl/ui/DropDown.cpp
  lib/mzgl/ui/FlexboxLayout.cpp
  lib/mzgl/ui/Layer.cpp
  lib/mzgl/ui/Layout.cpp
  lib/mzgl/ui/Scroller.cpp
  lib/mzgl/ui/ScrollingList.cpp
  lib/mzgl/ui/ScrollingListDeletable.cpp
  lib/mzgl/ui/ScrollingListItem.cpp
  lib/mzgl/ui/Slider.cpp
  lib/mzgl/ui/widgets/ReorderableGrid.cpp
  lib/mzgl/util/DateTime.cpp
  lib/mzgl/util/Dialogs.cpp
  lib/mzgl/util/Dylib.cpp
  lib/mzgl/util/errors.cpp
  lib/mzgl/util/FileWatcher.cpp
  lib/mzgl/util/FloatBuffer.cpp
  lib/mzgl/util/Haptics.cpp
  lib/mzgl/util/Int16Buffer.cpp
  lib/mzgl/util/jsonUtil.cpp
  lib/mzgl/util/jsonUtil.h
  lib/mzgl/util/log.cpp
  lib/mzgl/util/mainThread.cpp
  lib/mzgl/util/maths.cpp
  lib/mzgl/util/mzAssert.cpp
  lib/mzgl/util/ScopedUrl.cpp
  lib/mzgl/util/stringUtil.cpp
  lib/mzgl/util/UndoManager.cpp
  lib/mzgl/util/util.cpp)

if (NOT ANDROID)
  if (NOT BUILD_PLATFORM_IS_IOS)
    list (
      APPEND
      MZGL_SOURCES
      lib/mzgl/app/glfw/GLFWAppRunner.cpp
      lib/mzgl/app/glfw/DesktopWindowEventHandler.h
      lib/mzgl/app/glfw/DesktopWindowEventHandler.cpp
      lib/mzgl/app/glfw/DesktopWindowFileDragHandler.h
      lib/mzgl/app/glfw/GLFWOS.h)
  endif ()

  if (BUILD_PLATFORM_IS_IOS OR BUILD_PLATFORM_IS_MAC)
    list (
      APPEND
      MZGL_SOURCES
      lib/mzgl/audio/AudioFileApple.cpp
      lib/mzgl/app/apple/AppleWebView.mm
      lib/mzgl/app/apple/AUv3/AudioUnitViewController.mm
      lib/mzgl/app/apple/AUv3/MZGLEffectAU.mm
      lib/mzgl/app/web/MZWebView.cpp
      lib/mzgl/midi/AllMidiDevicesAppleImpl.mm
      lib/mzgl/midi/appleMidiUtils.cpp)
  endif ()

  if (BUILD_PLATFORM_IS_IOS)
    list (
      APPEND
      MZGL_SOURCES
      lib/mzgl/app/ios/iOSAppDelegate.h
      lib/mzgl/app/ios/iOSAppDelegate.mm
      lib/mzgl/app/ios/mainiOS.mm
      lib/mzgl/app/ios/MZGLKitView.h
      lib/mzgl/app/ios/MZGLKitView.mm
      lib/mzgl/app/ios/MZGLKitViewController.h
      lib/mzgl/app/ios/MZGLKitViewController.mm
      lib/mzgl/app/ios/UIBlockButton.h
      lib/mzgl/app/ios/UIBlockButton.m
      lib/mzgl/audio/iOS/AudioSystemIOS.h
      lib/mzgl/audio/iOS/AudioSystemIOS.mm
      lib/mzgl/midi/appleMidiUtils.cpp
      lib/mzgl/midi/appleMidiUtils.h)
  endif ()

  if (BUILD_PLATFORM_IS_MAC)
    list (
      APPEND
      MZGL_SOURCES
      lib/mzgl/audio/portaudio/PortAudioSystem.cpp
      lib/mzgl/app/glfw/DesktopWindowFileDragHandler_Mac.cpp
      lib/mzgl/app/glfw/GLFWOS_Mac.cpp
      lib/mzgl/app/mac/mainMac.mm
      lib/mzgl/app/mac/EventsView.mm
      lib/mzgl/app/mac/GL/MZGLView.mm
      lib/mzgl/app/mac/LayerExplorer.mm
      lib/mzgl/app/mac/MacAppDelegate.mm
      lib/mzgl/app/mac/MacFilePickerDelegate.mm
      lib/mzgl/app/mac/MacMenuBar.mm
      lib/mzgl/app/mac/TextConsole.mm
      lib/mzgl/app/web/MacWebView.mm
      lib/mzgl/app/apple/MZGLWebView.mm)
  endif ()

  if (BUILD_PLATFORM_IS_LINUX)
    list (
      APPEND
      MZGL_SOURCES
      lib/mzgl/audio/portaudio/PortAudioSystem.cpp
      lib/mzgl/app/glfw/DesktopWindowFileDragHandler_Linux.cpp
      lib/mzgl/app/glfw/GLFWOS_Linux.cpp
      lib/mzgl/util/linuxUtil.cpp
      lib/mzgl/app/glfw/mainGlfw.cpp
      lib/speex/speex_resampler.cpp
      lib/mzgl/audio/Resampler.cpp
      lib/mzgl/audio/AudioFileAndroid.cpp)
  endif ()

  if (BUILD_PLATFORM_IS_WINDOWS)
    list (
      APPEND
      MZGL_SOURCES
      lib/mzgl/app/glfw/DesktopWindowFileDragHandler_Windows.cpp
      lib/mzgl/app/glfw/GLFWOS_Windows.cpp
      lib/mzgl/util/winUtil.cpp
      lib/mzgl/util/AlignedAllocator.cpp
      lib/mzgl/app/glfw/mainGlfw.cpp)
  endif ()
endif ()

if (BUILD_PLATFORM_IS_ANDROID)
  list (
    APPEND
    MZGL_SOURCES
    lib/mzgl/app/android/mainAndroid.cpp
    lib/mzgl/app/android/androidUtil.cpp
    lib/mzgl/midi/AllMidiDevicesAndroidImpl.cpp
    lib/speex/speex_resampler.cpp
    lib/mzgl/audio/Resampler.cpp
    lib/mzgl/audio/AudioFileAndroid.cpp)
endif ()
