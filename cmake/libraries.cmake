set (
  MZGL_LIBRARIES
  concurrentqueue
  dr-libs
  elfxml
  fast-poly2tri
  ghcfilesystem
  glm::glm
  nlohmann_json::nlohmann_json
  readerwriterqueue
  stb
  yogacore
  zip-file)

if (NOT ANDROID AND NOT APPLE)
  find_package (OpenGL REQUIRED)
endif ()

if (BUILD_PLATFORM_IS_ANDROID)
  list (
    APPEND
    MZGL_LIBRARIES
    android
    native_app_glue
    EGL
    oboe
    GLESv3
    log
    mediandk)
endif ()

if (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
  list (APPEND MZGL_LIBRARIES z)
endif ()

if (BUILD_PLATFORM_IS_WINDOWS)
  list (
    APPEND
    MZGL_LIBRARIES
    winmm
    dsound
    ole32
    Urlmon
    Wininet)
endif ()

if (BUILD_PLATFORM_IS_LINUX)
  list (
    APPEND
    MZGL_LIBRARIES
    atomic
    GLESv2
    EGL)
endif ()

if (BUILD_PLATFORM_IS_MAC)
  list (
    APPEND
    MZGL_LIBRARIES
    glfw
    glew
    rtmidi
    portaudio)
endif ()

if (BUILD_PLATFORM_IS_IOS)
  list (APPEND MZGL_LIBRARIES AudioShareSDK)
  list (APPEND MZGL_LIBRARIES "-framework GLKit")
endif ()

mzgl_print_verbose_in_cyan ("[MZGL] -> Libraries are:")
foreach (lib IN LISTS MZGL_LIBRARIES)
  mzgl_print_verbose_in_cyan ("          ${lib}")
endforeach ()
