set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_STANDARD 14)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

# PRINT EVERYTHING!!!
set(CMAKE_VERBOSE_MAKEFILE ON)

set(MZGL_SOURCES
	${MZGL_ROOT}/lib/mzgl/animation/Tween.cpp
	${MZGL_ROOT}/lib/mzgl/audio/AudioSystem.cpp
	${MZGL_ROOT}/lib/mzgl/geom/Rectf.cpp
	${MZGL_ROOT}/lib/mzgl/geom/RoundedRect.cpp
	${MZGL_ROOT}/lib/mzgl/geom/SVG.cpp
	${MZGL_ROOT}/lib/mzgl/geom/Triangulator.cpp
	${MZGL_ROOT}/lib/mzgl/geom/3D/Camera.cpp
	${MZGL_ROOT}/lib/mzgl/gl/error.cpp
	${MZGL_ROOT}/lib/mzgl/gl/Fbo.cpp
	${MZGL_ROOT}/lib/mzgl/gl/Font.cpp
	${MZGL_ROOT}/lib/mzgl/gl/Graphics.cpp
	${MZGL_ROOT}/lib/mzgl/gl/Image-win.cpp
	${MZGL_ROOT}/lib/mzgl/gl/Image.cpp
	${MZGL_ROOT}/lib/mzgl/gl/Shader.cpp
	${MZGL_ROOT}/lib/mzgl/gl/SvgVbo.cpp
	${MZGL_ROOT}/lib/mzgl/gl/Texture.cpp
	${MZGL_ROOT}/lib/mzgl/gl/Vbo.cpp
	${MZGL_ROOT}/lib/mzgl/gl/drawing/Drawer.cpp
	${MZGL_ROOT}/lib/mzgl/gl/drawing/MitredLine.cpp
	${MZGL_ROOT}/lib/mzgl/midi/Midi.cpp
	${MZGL_ROOT}/lib/mzgl/midi/AllMidiDevices.cpp
	${MZGL_ROOT}/lib/mzgl/music/NotePattern.cpp
	${MZGL_ROOT}/lib/mzgl/music/scales.cpp
	${MZGL_ROOT}/lib/mzgl/ui/DropDown.cpp
	${MZGL_ROOT}/lib/mzgl/ui/Layer.cpp
	${MZGL_ROOT}/lib/mzgl/ui/Layout.cpp
	${MZGL_ROOT}/lib/mzgl/ui/Scroller.cpp
	${MZGL_ROOT}/lib/mzgl/ui/ScrollingList.cpp
	${MZGL_ROOT}/lib/mzgl/ui/ScrollingListDeletable.cpp
    ${MZGL_ROOT}/lib/mzgl/ui/ScrollingListItem.cpp
	${MZGL_ROOT}/lib/mzgl/ui/Slider.cpp
	${MZGL_ROOT}/lib/mzgl/ui/YogaLayout.cpp
	${MZGL_ROOT}/lib/mzgl/util/DateTime.cpp
	${MZGL_ROOT}/lib/mzgl/util/Dialogs.cpp
	${MZGL_ROOT}/lib/mzgl/util/Dylib.cpp
	${MZGL_ROOT}/lib/mzgl/util/errors.cpp
	${MZGL_ROOT}/lib/mzgl/util/events.cpp
	${MZGL_ROOT}/lib/mzgl/util/FileWatcher.cpp
	${MZGL_ROOT}/lib/mzgl/util/FloatBuffer.cpp
	${MZGL_ROOT}/lib/mzgl/util/Haptics.cpp
	${MZGL_ROOT}/lib/mzgl/util/log.cpp
	${MZGL_ROOT}/lib/mzgl/util/maths.cpp
	${MZGL_ROOT}/lib/mzgl/util/util.cpp
	${MZGL_ROOT}/lib/mzgl/util/ZipFile.cpp

)


set(HEADERS 
	${MZGL_ROOT}/lib/mzgl
	${MZGL_ROOT}/lib/mzgl/animation
	${MZGL_ROOT}/lib/mzgl/app/android
	${MZGL_ROOT}/lib/mzgl/app/mac
	${MZGL_ROOT}/lib/mzgl/app/glfw
	${MZGL_ROOT}/lib/mzgl/audio
	${MZGL_ROOT}/lib/mzgl/audio/portaudio
	${MZGL_ROOT}/lib/mzgl/file
	${MZGL_ROOT}/lib/mzgl/geom
	${MZGL_ROOT}/lib/mzgl/gl
	${MZGL_ROOT}/lib/mzgl/gl/drawing
	${MZGL_ROOT}/lib/mzgl/midi
	${MZGL_ROOT}/lib/mzgl/music
	${MZGL_ROOT}/lib/mzgl/ui
	${MZGL_ROOT}/lib/mzgl/util
	${MZGL_ROOT}/lib/glm
	${MZGL_ROOT}/lib
	${MZGL_ROOT}/lib/glfw/include/GLFW
	${MZGL_ROOT}/lib/glew/include/GL
	${MZGL_ROOT}/lib/glew/include
	${MZGL_ROOT}/lib/portaudio/include
	${MZGL_ROOT}/lib/fontstash
	${MZGL_ROOT}/lib/rtmidi
#	${MZGL_ROOT}/lib/PGMidi
	${MZGL_ROOT}/lib/pugixml/src
	${MZGL_ROOT}/lib/misc
	${MZGL_ROOT}/lib/zipper/zipper
    ${MZGL_ROOT}/lib/zipper/minizip
	${MZGL_ROOT}/lib/yoga
# 	${MZGL_ROOT}/lib/misc/mingw-std-threads

)

set(YOGA_SOURCES
    ${MZGL_ROOT}/lib/yoga/Utils.cpp
    ${MZGL_ROOT}/lib/yoga/YGConfig.cpp
    ${MZGL_ROOT}/lib/yoga/YGEnums.cpp
    ${MZGL_ROOT}/lib/yoga/YGLayout.cpp
    ${MZGL_ROOT}/lib/yoga/YGNode.cpp
    ${MZGL_ROOT}/lib/yoga/YGNodePrint.cpp
    ${MZGL_ROOT}/lib/yoga/YGStyle.cpp
    ${MZGL_ROOT}/lib/yoga/YGValue.cpp
    ${MZGL_ROOT}/lib/yoga/Yoga.cpp
    ${MZGL_ROOT}/lib/yoga/ylog.cpp
    ${MZGL_ROOT}/lib/yoga/event/event.cpp
)

# set(PGMIDI_SOURCES
# 	${MZGL_ROOT}/lib/PGMidi/PGMidi.mm		
# 	${MZGL_ROOT}/lib/PGMidi/PGMidiAllSources.mm	
# 	${MZGL_ROOT}/lib/PGMidi/PGMidiFind.mm
# )

set(BOOST_SOURCES
    ${MZGL_ROOT}/lib/boost/src/filesystem/utf8_codecvt_facet.cpp
    ${MZGL_ROOT}/lib/boost/src/filesystem/codecvt_error_category.cpp
    ${MZGL_ROOT}/lib/boost/src/filesystem/path_traits.cpp
    ${MZGL_ROOT}/lib/boost/src/filesystem/operations.cpp
    ${MZGL_ROOT}/lib/boost/src/filesystem/path.cpp
    ${MZGL_ROOT}/lib/boost/src/filesystem/unique_path.cpp
    ${MZGL_ROOT}/lib/boost/src/filesystem/windows_file_codecvt.cpp
    ${MZGL_ROOT}/lib/boost/src/filesystem/portability.cpp
)

set(ZIPPER_SOURCES
        ${MZGL_ROOT}/lib/zipper/minizip/ioapi_buf.c
        ${MZGL_ROOT}/lib/zipper/minizip/ioapi_mem.c
		${MZGL_ROOT}/lib/zipper/minizip/ioapi.c
        #${MZGL_ROOT}/lib/zipper/minizip/miniunz.c
       # ${MZGL_ROOT}/lib/zipper/minizip/minizip.c
        ${MZGL_ROOT}/lib/zipper/minizip/unzip.c
        ${MZGL_ROOT}/lib/zipper/minizip/zip.c
        ${MZGL_ROOT}/lib/zipper/zipper/CDirEntry.cpp
        ${MZGL_ROOT}/lib/zipper/zipper/tools.cpp
       # ${MZGL_ROOT}/lib/zipper/zipper/tps/dirent.c
        ${MZGL_ROOT}/lib/zipper/zipper/unzipper.cpp
        ${MZGL_ROOT}/lib/zipper/zipper/zipper.cpp
        )
if(WIN32)
	list(APPEND ZIPPER_SOURCES ${MZGL_ROOT}/lib/zipper/minizip/iowin32.c)
	list(APPEND HEADERS ${MZGL_ROOT}/lib/zipper/zipper/tps)

endif(WIN32)

add_library(zipper STATIC ${ZIPPER_SOURCES})
add_library(boostfs STATIC ${BOOST_SOURCES})
add_library(yoga STATIC ${YOGA_SOURCES})
add_library(pugixml OBJECT ${MZGL_ROOT}/lib/pugixml/src/pugixml.cpp)


include_directories(${HEADERS})

# Apple platforms have a special audio file reader
if(APPLE) 
	list(APPEND MZGL_SOURCES 
		${MZGL_ROOT}/lib/mzgl/audio/AudioFileApple.cpp
		${MZGL_ROOT}/lib/mzgl/midi/AllMidiDevicesAppleImpl.mm
		${MZGL_ROOT}/lib/mzgl/midi/appleMidiUtils.cpp
	)
else(APPLE)
	list(APPEND MZGL_SOURCES ${MZGL_ROOT}/lib/mzgl/audio/AudioFileAndroid.cpp)
endif(APPLE)

# if(APPLE) 
#    list(APPEND MZGL_SOURCES
#    ${MZGL_ROOT}/lib/mzgl/app/mac/mainMac.mm)
# endif(APPLE)

if(ANDROID)
###################################################################################################
## ANDROID SPECIFIC
###################################################################################################

list(APPEND MZGL_SOURCES
    ${MZGL_ROOT}/lib/mzgl/app/android/mainAndroid.cpp
    ${MZGL_ROOT}/lib/mzgl/app/android/androidUtil.cpp
)

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
include_directories(${ANDROID_NDK}/sources/android/native_app_glue/)


set(OBOE_DIR ${MZGL_ROOT}/lib/misc/oboe/)
add_subdirectory(${OBOE_DIR} ./oboe)
include_directories(
        ${OBOE_DIR}/include
        ${OBOE_DIR}/src
        )
else()
###################################################################################################
## ALL OTHER PLATFORMS (todo: merge this with android)
###################################################################################################
#set(CMAKE_C_COMPILER clang)
#set(CMAKE_CPP_COMPILER clang)


option(MZGL_UNIT_TEST "should this be a unit testing build - omits a main, so catch2 can take control" OFF)

list(APPEND MZGL_SOURCES
	${MZGL_ROOT}/lib/mzgl/app/glfw/GLFWAppRunner.cpp
	${MZGL_ROOT}/lib/mzgl/audio/portaudio/PortAudioSystem.cpp
)

# if(NOT MZGL_UNIT_TEST)
	list(APPEND MZGL_SOURCES
		${MZGL_ROOT}/lib/mzgl/app/glfw/mainGlfw.cpp)
# endif(NOT MZGL_UNIT_TEST)

if(MZGL_UNIT_TEST)
	add_compile_definitions(UNIT_TEST DEBUG=1)
	add_compile_options(-g)
endif(MZGL_UNIT_TEST)

set(PA_BUILD_STATIC ON) # CACHE BOOL "build portaudio static library")
set(PA_BUILD_SHARED OFF) # CACHE BOOL "build portaudio shared library")

if(WIN32)

list(APPEND MZGL_SOURCES
		${MZGL_ROOT}/lib/glew/src/glew.c
		#${MZGL_ROOT}/lib/mzgl/app/win64/mainWin.cpp
		${MZGL_ROOT}/lib/mzgl/util/winUtil.cpp
		${MZGL_ROOT}/lib/mzgl/util/AlignedAllocator.cpp
)
list(APPEND HEADERS
		${MZGL_ROOT}/lib/zipper/zipper/tps)

list(APPEND ZIPPER_SOURCES
		${MZGL_ROOT}/lib/zipper/zipper/tps/dirent.c)

include(${MZGL_ROOT}/CPM.cmake)
CPMAddPackage("gh:madler/zlib#cacf7f1")
CPMAddPackage("gh:PortAudio/portaudio#16884fc")

include_directories(${zlib_SOURCE_DIR})

# because zlib is managed by CPM on windows, I can't get zconf.h.cmakein to process to zconf.h
# so I just put it in the folder zlib-win - bit of a hack but whatevz
include_directories(${MZGL_ROOT}/lib/zlib-win)

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
	#${MZGL_ROOT}/lib/mzgl/app/linux/mainLinux.cpp
	${MZGL_ROOT}/lib/mzgl/util/linuxUtil.cpp
)
endif()





#set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${MZGL_ROOT}/lib-windows/release-${OF_PLATFORM}-${ARCH_BIT}")
#set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG   "${MZGL_ROOT}/lib-windows/debug-${OF_PLATFORM}-${ARCH_BIT}")


if(APPLE)
	# these are the files that have objective-c in them
	set_source_files_properties(
		${MZGL_ROOT}/lib/mzgl/App.cpp
		${MZGL_ROOT}/lib/mzgl/util/Dialogs.cpp
		${MZGL_ROOT}/lib/mzgl/util/util.cpp
		${MZGL_ROOT}/lib/mzgl/audio/AudioFileApple.cpp
		PROPERTIES COMPILE_FLAGS "-x objective-c++"
	)

	add_compile_definitions(GL_SILENCE_DEPRECATION MZGL_GL2 MZGL_MAC_GLFW __MACOSX_CORE__)
	# add_library(PGMidi STATIC ${PGMIDI_SOURCES})

	list(APPEND MZGL_SOURCES ${MZGL_ROOT}/lib/mzgl/app/mac/MacMenuBar.mm)
	list(APPEND MZGL_SOURCES ${MZGL_ROOT}/lib/mzgl/app/mac/LayerExplorer.mm)
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


if(WIN32) 
add_executable(${APP_NAME} WIN32 ${${APP_NAME}_SOURCES})
else(WIN32)
add_executable(${APP_NAME} ${${APP_NAME}_SOURCES})

endif()
add_library(libmzgl STATIC ${MZGL_SOURCES})
add_library(libmzgl_unit_test STATIC ${MZGL_SOURCES})
target_compile_definitions(libmzgl_unit_test PUBLIC -DUNIT_TEST -DDEBUG)


add_subdirectory(${MZGL_ROOT}/lib/glfw "glfw-build")
if(WIN32)
	# windows portaudio: can't seem to get the cmake build working (WASAPI doesn't work)
	# so I built it in visual studio and am attempting to link it here (need to copy dll into bin)
	#include_directories(${MZGL_ROOT}/lib/portaudio/build/msvc/Win32/Release/) ## this line may not be needed
	#add_library(portaudio_x86 SHARED ${MZGL_ROOT}/lib/portaudio/build/msvc/x64/Release/portaudio_x86.lib)
	#set_target_properties(portaudio_x86 PROPERTIES LINKER_LANGUAGE C)
#	these 2 apis don't seem to compile with clang-cl for now, can't be bothered to work it out
#	set(PA_USE_WASAPI OFF)
#	set(PA_USE_WDMKS OFF)
else(WIN32)
add_subdirectory(${MZGL_ROOT}/lib/portaudio "portaudio-build")
endif()

add_library(RtMidi OBJECT ${MZGL_ROOT}/lib/rtmidi/RtMidi.cpp)


find_package(OpenGL REQUIRED)


add_dependencies(libmzgl pugixml)
add_dependencies(libmzgl RtMidi)

add_dependencies(${APP_NAME} libmzgl)

set_target_properties( ${APP_NAME}
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY    ${PROJECT_SOURCE_DIR}/bin
)


set(LIBS
	libmzgl
	zipper
    pugixml
    RtMidi
    glfw
    boostfs
    yoga
    blahdio
)


if(WIN32) 
	list(APPEND LIBS 
		${GLFW_LIBRARIES}
	    ${OPENGL_gl_LIBRARY}
		winmm
		dsound
	    ole32
		zlib
		portaudio_static
	)
else(WIN32)
	list(APPEND LIBS z portaudio_static)
endif()

if(LINUX)

	FIND_PACKAGE(PkgConfig REQUIRED)
	PKG_CHECK_MODULES(GTK REQUIRED gtk+-3.0)

	# Setup CMake to use GTK+, tell the compiler where to look for headers
	# and to the linker where to look for libraries
	INCLUDE_DIRECTORIES(${GTK_INCLUDE_DIRS})
	LINK_DIRECTORIES(${GTK_LIBRARY_DIRS})

	# Add other flags to the compiler
	ADD_DEFINITIONS(${GTK_CFLAGS_OTHER})

	list(APPEND LIBS
		jack 
		atomic # needed for raspberry pi
		GLESv2
		EGL
		${GTK_LIBRARIES}
		${ALSA_LIBRARIES}
	)
endif(LINUX)

# if(APPLE)
# 	list(APPEND LIBS PGMidi)
# endif(APPLE)

target_link_libraries (${APP_NAME} ${LIBS})

if(APPLE)
	target_link_libraries(${APP_NAME} "-framework OpenGL -framework Accelerate -framework AppKit -framework CoreMidi -framework AVFoundation -framework CoreMedia")
endif(APPLE)

endif() ## ALL OTHER PLATFORMS
