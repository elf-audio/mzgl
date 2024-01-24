function (mzgl_lock_cpm_cache)
  if (CPM_SOURCE_CACHE)
    file (LOCK ${CPM_SOURCE_CACHE} DIRECTORY GUARD FILE)
  endif ()
endfunction ()

function (mzgl_unlock_cpm_cache)
  if (CPM_SOURCE_CACHE)
    file (LOCK ${CPM_SOURCE_CACHE} DIRECTORY GUARD FILE RELEASE)
  endif ()
endfunction ()

function (mzgl_add_package PACKAGE_NAME)
  mzgl_print_in_yellow ("[CPM] -> Adding package ${PACKAGE_NAME}")
  set (_saved_CMAKE_MESSAGE_LOG_LEVEL ${CMAKE_MESSAGE_LOG_LEVEL})
  set (CMAKE_MESSAGE_LOG_LEVEL NOTICE)
  cpmaddpackage (${PACKAGE_NAME})
  set ("${CPM_LAST_PACKAGE_NAME}_DEPS_DIR" "${${CPM_LAST_PACKAGE_NAME}_SOURCE_DIR}"
       CACHE INTERNAL "${CPM_LAST_PACKAGE_NAME} Deps dir")
  set (CMAKE_MESSAGE_LOG_LEVEL ${_saved_CMAKE_MESSAGE_LOG_LEVEL})
  mzgl_print_verbose_in_yellow ("      -> Package name is ${CPM_LAST_PACKAGE_NAME}")
endfunction ()

function (
  mzgl_add_named_package
  DEVELOPER
  NAME
  SHA
  TARGET
  INCLUDE_SUFFIX)
  mzgl_print_in_yellow ("[CPM] -> Adding package gh:${DEVELOPER}/${NAME}#${SHA}")

  set (_saved_CMAKE_MESSAGE_LOG_LEVEL ${CMAKE_MESSAGE_LOG_LEVEL})
  set (CMAKE_MESSAGE_LOG_LEVEL NOTICE)

  cpmaddpackage (
    NAME
    ${TARGET}
    URL
    https://github.com/${DEVELOPER}/${NAME}/archive/${SHA}.zip
    DOWNLOAD_ONLY
    True)

  add_library (${TARGET} INTERFACE IMPORTED GLOBAL)
  target_include_directories (${TARGET} SYSTEM INTERFACE ${${TARGET}_SOURCE_DIR})
  include_directories (SYSTEM INTERFACE ${${TARGET}_SOURCE_DIR})

  if (NOT ${INCLUDE_SUFFIX} STREQUAL "")
    target_include_directories (${TARGET} SYSTEM INTERFACE ${${TARGET}_SOURCE_DIR}/${INCLUDE_SUFFIX})
    include_directories (${TARGET} SYSTEM INTERFACE ${${TARGET}_SOURCE_DIR}/${INCLUDE_SUFFIX})
  endif ()

  set (ENV{${TARGET}_SOURCE_DIR} "${TARGET}_SOURCE_DIR}")
  set ("${CPM_LAST_PACKAGE_NAME}_DEPS_DIR" "${${CPM_LAST_PACKAGE_NAME}_SOURCE_DIR}"
       CACHE INTERNAL "${CPM_LAST_PACKAGE_NAME} Deps dir")
  set (CMAKE_MESSAGE_LOG_LEVEL ${_saved_CMAKE_MESSAGE_LOG_LEVEL})
  mzgl_print_verbose_in_yellow ("      -> Package name is ${CPM_LAST_PACKAGE_NAME}")
endfunction ()

function (
  mzgl_add_named_package_and_library
  DEVELOPER
  NAME
  SHA
  TARGET
  INCLUDE_SUFFIX
  SOURCES)
  mzgl_add_named_package (
    ${DEVELOPER}
    ${NAME}
    ${SHA}
    ${TARGET}
    ${INCLUDE_SUFFIX})
  list (TRANSFORM SOURCES PREPEND ENV{${TARGET}_SOURCE_DIR})
  add_library (${TARGET} STATIC ${SOURCES})
endfunction ()

function (mzgl_add_audioshare)
  mzgl_print_in_yellow ("[CPM] -> Adding package gh:lijon/AudioShareSDK#995a85b")
  set (_saved_CMAKE_MESSAGE_LOG_LEVEL ${CMAKE_MESSAGE_LOG_LEVEL})
  set (CMAKE_MESSAGE_LOG_LEVEL NOTICE)
  cpmaddpackage (
    NAME
    AudioShareSDK
    URL
    https://github.com/lijon/AudioShareSDK/archive/995a85b.zip
    DOWNLOAD_ONLY
    True)
  add_library (AudioShareSDK STATIC ${AudioShareSDK_SOURCE_DIR}/AudioShareSDK.h
                                    ${AudioShareSDK_SOURCE_DIR}/AudioShareSDK.m)
  set (CMAKE_MESSAGE_LOG_LEVEL ${_saved_CMAKE_MESSAGE_LOG_LEVEL})
endfunction ()

function (mzgl_add_portaudio)
  if (NOT ANDROID AND NOT IOS)
    mzgl_add_package ("gh:PortAudio/portaudio@19.7.0" DOWNLOAD_ONLY True)
  endif ()
endfunction ()

function (mzgl_add_rtmidi)
  if (NOT ANDROID AND NOT IOS)
    mzgl_add_package ("gh:thestk/rtmidi#6ad594f" DOWNLOAD_ONLY True)
  endif ()
endfunction ()

function (mzgl_add_packages)
  mzgl_lock_cpm_cache ()

  mzgl_add_package ("gh:nlohmann/json@3.11.3")
  mzgl_add_package ("gh:facebook/yoga@2.0.1")
  mzgl_add_package ("gh:cameron314/concurrentqueue@1.0.4")
  mzgl_add_package ("gh:cameron314/readerwriterqueue@1.0.6")
  mzgl_add_package ("gh:g-truc/glm#0.9.9.8")
  mzgl_add_package ("gh:glfw/glfw#3.3.9")
  mzgl_add_package ("gh:sebastiandev/zipper#87b14a4")
  mzgl_add_package ("gh:elf-audio/fontstash#f49c10f")
  mzgl_add_package ("gh:zeux/pugixml@1.14")

  mzgl_add_named_package (
    "gulrak"
    "filesystem"
    "cd6805e"
    "ghcfilesystem"
    "include")
  mzgl_add_named_package (
    "nothings"
    "stb"
    "f4a71b1"
    "stb"
    "")
  mzgl_add_named_package (
    "mackron"
    "dr_libs"
    "e4a7765"
    "dr-libs"
    "")
  mzgl_add_named_package (
    "elf-audio"
    "fast-poly2tri"
    "5dcc516"
    "fast-poly2tri"
    "")

  mzgl_add_audioshare ()
  mzgl_add_portaudio ()
  mzgl_add_rtmidi ()

  mzgl_unlock_cpm_cache ()
endfunction ()
