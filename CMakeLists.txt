set(CMAKE_VERBOSE_MAKEFILE ON)

if(ANDROID)
set(CMAKE_CXX_STANDARD 17)
else()
set(CMAKE_CXX_STANDARD 14)
endif()

set(CMAKE_CXX_STANDARD_REQUIRED ON)

# PRINT EVERYTHING!!!
set(CMAKE_VERBOSE_MAKEFILE ON)

set(MZGL_SOURCES
	lib/mzgl/animation/Tween.cpp
	lib/mzgl/audio/AudioSystem.cpp
	lib/mzgl/geom/Rectf.cpp
	lib/mzgl/geom/RoundedRect.cpp
	lib/mzgl/geom/SVG.cpp
	lib/mzgl/geom/Triangulator.cpp
	lib/mzgl/geom/3D/Camera.cpp
	lib/mzgl/gl/error.cpp
	lib/mzgl/gl/Fbo.cpp
	lib/mzgl/gl/Font.cpp
	lib/mzgl/gl/Graphics.cpp
	lib/mzgl/gl/Image-win.cpp
	lib/mzgl/gl/Image.cpp
	lib/mzgl/gl/Shader.cpp
	lib/mzgl/gl/SvgVbo.cpp
	lib/mzgl/gl/Texture.cpp
	lib/mzgl/gl/Vbo.cpp
	lib/mzgl/gl/drawing/Drawer.cpp
	lib/mzgl/gl/drawing/MitredLine.cpp
	lib/mzgl/midi/Midi.cpp
	lib/mzgl/midi/AllMidiDevices.cpp
	lib/mzgl/music/NotePattern.cpp
	lib/mzgl/music/scales.cpp
	lib/mzgl/ui/DropDown.cpp
	lib/mzgl/ui/Layer.cpp
	lib/mzgl/ui/Layout.cpp
	lib/mzgl/ui/Scroller.cpp
	lib/mzgl/ui/ScrollingList.cpp
	lib/mzgl/ui/ScrollingListDeletable.cpp
    lib/mzgl/ui/ScrollingListItem.cpp
	lib/mzgl/ui/Slider.cpp
	lib/mzgl/ui/YogaLayout.cpp
	lib/mzgl/util/DateTime.cpp
	lib/mzgl/util/Dialogs.cpp
	lib/mzgl/util/Dylib.cpp
	lib/mzgl/util/errors.cpp
	lib/mzgl/util/events.cpp
	lib/mzgl/util/FileWatcher.cpp
	lib/mzgl/util/FloatBuffer.cpp
	lib/mzgl/util/Haptics.cpp
	lib/mzgl/util/log.cpp
	lib/mzgl/util/maths.cpp
	lib/mzgl/util/util.cpp
	lib/mzgl/util/ZipFile.cpp

)

# file( GLOB HEADERS  

# 	lib/mzgl/*.h
# 	lib/mzgl/animation/*.h
# 	lib/mzgl/app/android/*.h
# 	lib/mzgl/app/mac/*.h
# 	lib/mzgl/app/glfw/*.h
# 	lib/mzgl/audio/*.h
# 	lib/mzgl/audio/portaudio/*.h
# 	lib/mzgl/file/*.h
# 	lib/mzgl/geom/*.h
# 	lib/mzgl/gl/*.h
# 	lib/mzgl/gl/drawing/*.h
# 	lib/mzgl/midi/*.h
# 	lib/mzgl/music/*.h
# 	lib/mzgl/ui/*.h
# 	lib/mzgl/util/*.h
# 	lib/glm/*.h
# 	lib/*.h
# 	lib/glfw/include/GLFW/*.h
# 	lib/glew/include/GL/*.h
# 	lib/glew/include/*.h
# 	lib/portaudio/include/*.h
# 	lib/fontstash/*.h
# 	lib/rtmidi/*.h
# 	lib/pugixml/src/*.h
# 	lib/misc/*.h
# 	lib/zipper/zipper/*.h
#     lib/zipper/minizip/*.h
# 	lib/yoga/*.h
# )


set(HEADERS 
	lib/mzgl
	lib/mzgl/animation
	lib/mzgl/app/android
	lib/mzgl/app/mac
	lib/mzgl/app/glfw
	lib/mzgl/audio
	lib/mzgl/audio/portaudio
	lib/mzgl/file
	lib/mzgl/geom
	lib/mzgl/gl
	lib/mzgl/gl/drawing
	lib/mzgl/midi
	lib/mzgl/music
	lib/mzgl/ui
	lib/mzgl/util
	lib/glm
	lib
	lib/glfw/include/GLFW
	lib/glew/include/GL
	lib/glew/include
	lib/portaudio/include
	lib/fontstash
	lib/rtmidi
	lib/pugixml/src
	lib/misc
	lib/zipper/zipper
    lib/zipper/minizip
	lib/yoga
)

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
    lib/yoga/event/event.cpp
)

set(BOOST_SOURCES
    lib/boost/src/filesystem/utf8_codecvt_facet.cpp
    lib/boost/src/filesystem/codecvt_error_category.cpp
    lib/boost/src/filesystem/path_traits.cpp
    lib/boost/src/filesystem/operations.cpp
    lib/boost/src/filesystem/path.cpp
    lib/boost/src/filesystem/unique_path.cpp
    lib/boost/src/filesystem/windows_file_codecvt.cpp
    lib/boost/src/filesystem/portability.cpp
)

set(ZIPPER_SOURCES
      lib/zipper/minizip/ioapi_buf.c
      lib/zipper/minizip/ioapi_mem.c
		lib/zipper/minizip/ioapi.c
        #lib/zipper/minizip/miniunz.c
       # lib/zipper/minizip/minizip.c
       lib/zipper/minizip/unzip.c
       lib/zipper/minizip/zip.c
       lib/zipper/zipper/CDirEntry.cpp
       lib/zipper/zipper/tools.cpp
       # lib/zipper/zipper/tps/dirent.c
        lib/zipper/zipper/unzipper.cpp
        lib/zipper/zipper/zipper.cpp
        )
if(WIN32)
	list(APPEND ZIPPER_SOURCES lib/zipper/minizip/iowin32.c)
	list(APPEND HEADERS lib/zipper/zipper/tps)

endif(WIN32)

add_library(zipper STATIC ${ZIPPER_SOURCES})
add_library(boostfs STATIC ${BOOST_SOURCES})
add_library(yoga STATIC ${YOGA_SOURCES})
add_library(pugixml OBJECT lib/pugixml/src/pugixml.cpp)


# set(OPENSSL_BRANCH OpenSSL_1_1_1m)
# set(BUILD_OPENSSL ON)
# set(OPENSSL_BUILD_VERSION 1.1.1m)
# add_subdirectory(lib/openssl-cmake)




# Apple platforms have a special audio file reader
if(APPLE) 
	list(APPEND MZGL_SOURCES 
		lib/mzgl/audio/AudioFileApple.cpp
		lib/mzgl/midi/AllMidiDevicesAppleImpl.mm
		lib/mzgl/midi/appleMidiUtils.cpp
	)
else(APPLE)
	list(APPEND MZGL_SOURCES lib/mzgl/audio/AudioFileAndroid.cpp)
endif(APPLE)

# if(APPLE) 
#    list(APPEND MZGL_SOURCES
#    lib/mzgl/app/mac/mainMac.mm)
# endif(APPLE)

if(ANDROID)
###################################################################################################
## ANDROID SPECIFIC
###################################################################################################

list(APPEND MZGL_SOURCES
    lib/mzgl/app/android/mainAndroid.cpp
    lib/mzgl/app/android/androidUtil.cpp
)
include_directories(${ANDROID_NDK}/sources/android/native_app_glue/)

add_library(libmzgl STATIC ${MZGL_SOURCES})
add_library(libmzgl_unit_test STATIC ${MZGL_SOURCES})
target_compile_definitions(libmzgl_unit_test PUBLIC -DUNIT_TEST -DDEBUG)


# build native_app_glue as a static lib

#is this line needed??
set(${CMAKE_C_FLAGS}, "${CMAKE_C_FLAGS}")

add_library(native_app_glue STATIC
        ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c)


#-DAUTO_TEST
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -Ofast -fno-stack-protector")

# Export ANativeActivity_onCreate(),
# Refer to: https://github.com/android-ndk/ndk/issues/381.
set(CMAKE_SHARED_LINKER_FLAGS
        "${CMAKE_SHARED_LINKER_FLAGS} -u ANativeActivity_onCreate")

# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds them for you.
# Gradle automatically packages shared libraries with your APK.


else()
###################################################################################################
## ALL OTHER PLATFORMS (todo: merge this with android)
###################################################################################################
#set(CMAKE_C_COMPILER clang)
#set(CMAKE_CPP_COMPILER clang)


option(MZGL_UNIT_TEST "should this be a unit testing build - omits a main, so catch2 can take control" OFF)

list(APPEND MZGL_SOURCES
	lib/mzgl/app/glfw/GLFWAppRunner.cpp
	lib/mzgl/audio/portaudio/PortAudioSystem.cpp
)

# if(NOT MZGL_UNIT_TEST)
	list(APPEND MZGL_SOURCES
		lib/mzgl/app/glfw/mainGlfw.cpp)
# endif(NOT MZGL_UNIT_TEST)

if(MZGL_UNIT_TEST)
	add_compile_definitions(UNIT_TEST DEBUG=1)
	add_compile_options(-g)
endif(MZGL_UNIT_TEST)

set(PA_BUILD_STATIC ON) # CACHE BOOL "build portaudio static library")
set(PA_BUILD_SHARED OFF) # CACHE BOOL "build portaudio shared library")

if(WIN32)

list(APPEND MZGL_SOURCES
		lib/glew/src/glew.c
		#lib/mzgl/app/win64/mainWin.cpp
		lib/mzgl/util/winUtil.cpp
		lib/mzgl/util/AlignedAllocator.cpp
)
list(APPEND HEADERS
		lib/zipper/zipper/tps)

list(APPEND ZIPPER_SOURCES
		lib/zipper/zipper/tps/dirent.c)

include(CPM.cmake)
CPMAddPackage("gh:madler/zlib#cacf7f1")
CPMAddPackage("gh:PortAudio/portaudio#16884fc")
CPMAddPackage("gh:yhirose/cpp-httplib#412ab5f")
include_directories(${zlib_SOURCE_DIR})

# because zlib is managed by CPM on windows, I can't get zconf.h.cmakein to process to zconf.h
# so I just put it in the folder zlib-win - bit of a hack but whatevz
include_directories(lib/zlib-win)

	add_compile_definitions(_WIN32_WINNT=0x0501 GLEW_STATIC)
	add_compile_definitions(__WINDOWS_WASAPI__)
	add_compile_definitions(__WINDOWS_MM__)
	add_compile_definitions(PA_USE_WASAPI=1)
	add_compile_definitions(NOMINMAX)
# don't need c++14 because I think it's set by cmake, same with O3
	#add_compile_options(-std:c++14 -O3)
# https://forum.juce.com/t/cmake-cannot-use-try-with-exceptions-disabled/40983
# /MT embeds static runtime
set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} " -EHsc /MT -Xclang -fexceptions -Xclang -fcxx-exceptions ")
set(CMAKE_C_FLAGS ${CMAKE_C_FLAGS} " -EHsc /MT -Xclang -fexceptions -Xclang -fcxx-exceptions ")

# this lets clang-cl have exceptions as per https://lists.llvm.org/pipermail/llvm-dev/2015-September/089974.html
#set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} " -Xclang -fexceptions -Xclang -fcxx-exceptions ")#-Xclang -fnew-ms-eh ")
#set(CMAKE_EXE_LINKER_FLAGS ${CMAKE_EXE_LINKER_FLAGS} " -Xlinker /subsystem:windows ")
endif(WIN32)

if(UNIX AND NOT APPLE)
	set(LINUX TRUE)
endif()

if(LINUX)
list(APPEND MZGL_SOURCES
	#lib/mzgl/app/linux/mainLinux.cpp
	lib/mzgl/util/linuxUtil.cpp
)
endif()





#set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE "lib-windows/release-${OF_PLATFORM}-${ARCH_BIT}")
#set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG   "lib-windows/debug-${OF_PLATFORM}-${ARCH_BIT}")


if(APPLE)
	# these are the files that have objective-c in them
	set_source_files_properties(
		lib/mzgl/App.cpp
		lib/mzgl/util/Dialogs.cpp
		lib/mzgl/util/util.cpp
		lib/mzgl/audio/AudioFileApple.cpp
		PROPERTIES COMPILE_FLAGS "-x objective-c++"
	)

	add_compile_definitions(GL_SILENCE_DEPRECATION MZGL_GL2 MZGL_MAC_GLFW __MACOSX_CORE__)
	# add_library(PGMidi STATIC ${PGMIDI_SOURCES})

	list(APPEND MZGL_SOURCES lib/mzgl/app/mac/MacMenuBar.mm)
	list(APPEND MZGL_SOURCES lib/mzgl/app/mac/LayerExplorer.mm)
endif(APPLE)



if(LINUX)
    add_compile_definitions(__LINUX_ALSA__)
    find_package(ALSA REQUIRED)
    
 #-mfpu=neon-vfpv4
    if(__ARMEL__)
        add_compile_options(-mcpu=cortex-a7 -mfloat-abi=hard -mfpu=neon -O3)

    else(__ARMEL__)
 #   	add_compile_options(-O3)
 #		add_compile_options(-g -O0)
    endif(__ARMEL__)

    #probably want pulseaudio because the latency is supposedly better
       # add_compile_definitions(__LINUX_PULSE__)
endif(LINUX)



add_library(libmzgl STATIC ${MZGL_SOURCES})
add_library(libmzgl_unit_test STATIC ${MZGL_SOURCES})

target_include_directories(libmzgl PUBLIC ${HEADERS})
target_include_directories(libmzgl_unit_test PUBLIC ${HEADERS})
include_directories(libmzgl PUBLIC ${HEADERS})
include_directories(libmzgl_unit_test PUBLIC ${HEADERS})

target_compile_definitions(libmzgl_unit_test PUBLIC -DUNIT_TEST -DDEBUG)


add_subdirectory(lib/glfw "glfw-build")
if(WIN32)
	# windows portaudio: can't seem to get the cmake build working (WASAPI doesn't work)
	# so I built it in visual studio and am attempting to link it here (need to copy dll into bin)
	#include_directories(lib/portaudio/build/msvc/Win32/Release/) ## this line may not be needed
	#add_library(portaudio_x86 SHARED lib/portaudio/build/msvc/x64/Release/portaudio_x86.lib)
	#set_target_properties(portaudio_x86 PROPERTIES LINKER_LANGUAGE C)
#	these 2 apis don't seem to compile with clang-cl for now, can't be bothered to work it out
#	set(PA_USE_WASAPI OFF)
#	set(PA_USE_WDMKS OFF)
else(WIN32)
add_subdirectory(lib/portaudio "portaudio-build")
endif()

add_library(RtMidi OBJECT lib/rtmidi/RtMidi.cpp)
# add_subdirectory("lib/RtMidi" "rtmidi-build")

find_package(OpenGL REQUIRED)


add_dependencies(libmzgl pugixml)
add_dependencies(libmzgl RtMidi)




endif() ## ALL OTHER PLATFORMS
