# TODO: can make a CPM of nlohmann json, moodycamel stuff
# TODO: swap out boost fs for https://github.com/gulrak/filesystem

set(CMAKE_VERBOSE_MAKEFILE ON)
cmake_minimum_required(VERSION 3.18)
project(mzgl)

# makes clang output colours
add_compile_options (-fcolor-diagnostics)
if(ANDROID OR WIN32)
set(CMAKE_CXX_STANDARD 17)
else()
set(CMAKE_CXX_STANDARD 14)
endif()

set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(CPM.cmake)

CPMAddPackage("gh:yhirose/cpp-httplib#412ab5f")
# CPMAddPackage("gh:gulrak/filesystem#f437344")

# CPMAddPackage(
#   NAME nlohmann_json
#   VERSION 3.9.1
#   OPTIONS 
#     "JSON_BuildTests OFF"
# )


if(CMAKE_BUILD_TYPE MATCHES DEBUG)
    add_compile_definitions(DEBUG=1)

endif()

set(LINUX FALSE)
if(NOT ANDROID AND UNIX AND NOT APPLE)
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
		lib/mzgl/audio/DrAudioFileReader.cpp
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



foreach(source IN LISTS MZGL_SOURCES)
    get_filename_component(source_path "${source}" PATH)
    string(REPLACE "/" "\\" source_path_group "${source_path}")
    source_group("${source_path_group}" FILES "${source}")
endforeach()

set(HEADERS 
	lib/mzgl
	lib/mzgl/animation
	lib/mzgl/app/android
	lib/mzgl/app/mac
	lib/mzgl/app/mac/GL
	lib/mzgl/app/glfw
	lib/mzgl/audio
	lib/mzgl/audio/portaudio
	lib/mzgl/audio/iOS
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
	lib/zipper/minizip/unzip.c
	lib/zipper/minizip/zip.c
	lib/zipper/zipper/CDirEntry.cpp
	lib/zipper/zipper/tools.cpp
	lib/zipper/zipper/unzipper.cpp
	lib/zipper/zipper/zipper.cpp
 )

if(WIN32)
	list(APPEND ZIPPER_SOURCES lib/zipper/minizip/iowin32.c)
	list(APPEND HEADERS lib/zipper/zipper/tps)
endif()

option(MZGL_UNIT_TEST "should this be a unit testing build - omits a main, so catch2 can take control" OFF)

if(MZGL_UNIT_TEST)
	add_compile_definitions(UNIT_TEST DEBUG=1)
	add_compile_options(-g)
endif(MZGL_UNIT_TEST)



add_library(zipper STATIC ${ZIPPER_SOURCES})
add_library(boostfs STATIC ${BOOST_SOURCES})
add_library(yoga STATIC ${YOGA_SOURCES})
add_library(pugixml OBJECT lib/pugixml/src/pugixml.cpp)


# add_dependencies(libmzgl pugixml)

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
		lib/mzgl/app/mac/MacMenuBar.mm
		lib/mzgl/app/mac/EventsView.mm
		lib/mzgl/app/mac/GL/MZGLView.mm
		lib/mzgl/app/mac/LayerExplorer.mm
		lib/mzgl/app/mac/TextConsole.mm
	)

	# these are the files that have objective-c in them
	set_source_files_properties(
		lib/mzgl/App.cpp
		lib/mzgl/util/Dialogs.cpp
		lib/mzgl/util/util.cpp
		lib/mzgl/audio/AudioFileApple.cpp
		PROPERTIES COMPILE_FLAGS "-x objective-c++"
	)
else()
	list(APPEND MZGL_SOURCES lib/misc/speex_resampler.cpp)
	list(APPEND MZGL_SOURCES lib/mzgl/audio/Resampler.cpp)
	list(APPEND MZGL_SOURCES lib/mzgl/audio/AudioFileAndroid.cpp)
endif()


if(ANDROID)

	list(APPEND MZGL_SOURCES
	    lib/mzgl/app/android/mainAndroid.cpp
	    lib/mzgl/app/android/androidUtil.cpp
	)
else()
	list(APPEND MZGL_SOURCES
		lib/mzgl/app/glfw/GLFWAppRunner.cpp
		lib/mzgl/audio/portaudio/PortAudioSystem.cpp
		lib/mzgl/app/glfw/mainGlfw.cpp
	)
endif()

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
endif()



if(LINUX)
	list(APPEND MZGL_SOURCES
		#lib/mzgl/app/linux/mainLinux.cpp
		lib/mzgl/util/linuxUtil.cpp
	)
endif()

add_library(libmzgl STATIC ${MZGL_SOURCES})
add_library(libmzgl_unit_test STATIC ${MZGL_SOURCES})
target_include_directories(libmzgl PUBLIC ${HEADERS})
target_include_directories(libmzgl_unit_test PUBLIC ${HEADERS})
include_directories(libmzgl PUBLIC ${HEADERS})
include_directories(libmzgl_unit_test PUBLIC ${HEADERS})
target_compile_definitions(libmzgl_unit_test PUBLIC -DUNIT_TEST -DDEBUG)


set(MZGL_LIBS
	zipper 
 	boostfs 
	yoga 
	pugixml
)


if(WIN32)
	list(APPEND MZGL_LIBS zlib)
	target_include_directories(libmzgl PUBLIC lib/zlib-win)
else()
	list(APPEND MZGL_LIBS z)
endif()

set(PA_BUILD_STATIC ON) # CACHE BOOL "build portaudio static library")
set(PA_BUILD_SHARED OFF) # CACHE BOOL "build portaudio shared library")

	#set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE "lib-windows/release-${OF_PLATFORM}-${ARCH_BIT}")
	#set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG   "lib-windows/debug-${OF_PLATFORM}-${ARCH_BIT}")

if(NOT ANDROID) 
	add_subdirectory(lib/glfw "glfw-build")

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

	add_library(native_app_glue STATIC
	        ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c)

	# find_package (oboe REQUIRED CONFIG)



	set (OBOE_DIR ../../android/oboe)
	add_subdirectory (${OBOE_DIR} ./oboe)
	include_directories (${OBOE_DIR}/include)





	#-DAUTO_TEST
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -Ofast -fno-stack-protector")

	# Export ANativeActivity_onCreate(),
	# Refer to: https://github.com/android-ndk/ndk/issues/381.
	set(CMAKE_SHARED_LINKER_FLAGS
	        "${CMAKE_SHARED_LINKER_FLAGS} -u ANativeActivity_onCreate")
	list(APPEND MZGL_LIBS
		# android specific
		android
		native_app_glue
		EGL
		# oboe::oboe
		oboe
		GLESv3
		log
		mediandk
	)

elseif(APPLE)
	add_compile_definitions(GL_SILENCE_DEPRECATION MZGL_GL2 MZGL_MAC_GLFW __MACOSX_CORE__)
	target_link_libraries(libmzgl "-framework OpenGL -framework Accelerate -framework AppKit -framework CoreVideo -framework CoreMidi -framework CoreAudio -framework AVFoundation -framework CoreMedia")

elseif(WIN32)

	CPMAddPackage("gh:madler/zlib#cacf7f1")
	CPMAddPackage("gh:PortAudio/portaudio#16884fc")
	
	include_directories(${zlib_SOURCE_DIR})

	# because zlib is managed by CPM on windows, I can't get zconf.h.cmakein to process to zconf.h
	# so I just put it in the folder zlib-win - bit of a hack but whatevz
	include_directories(lib/zlib-win)

	add_compile_definitions(_WIN32_WINNT=0x0501 GLEW_STATIC)
	add_compile_definitions(__WINDOWS_WASAPI__)
	add_compile_definitions(__WINDOWS_MM__)
	add_compile_definitions(PA_USE_WASAPI=1)
	add_compile_definitions(NOMINMAX)
	# set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS")
	# don't need c++14 because I think it's set by cmake, same with O3
		#add_compile_options(-std:c++14 -O3)
	# https://forum.juce.com/t/cmake-cannot-use-try-with-exceptions-disabled/40983
	# /MT embeds static runtime
	set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} " -EHsc /MT -Xclang -fexceptions -Xclang -fcxx-exceptions ")
	# set(CMAKE_C_FLAGS ${CMAKE_C_FLAGS} " -EHsc /MT -Xclang -fexceptions -Xclang -fcxx-exceptions ")

	# this lets clang-cl have exceptions as per https://lists.llvm.org/pipermail/llvm-dev/2015-September/089974.html
	#set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} " -Xclang -fexceptions -Xclang -fcxx-exceptions ")#-Xclang -fnew-ms-eh ")
	#set(CMAKE_EXE_LINKER_FLAGS ${CMAKE_EXE_LINKER_FLAGS} " -Xlinker /subsystem:windows ")

	list(APPEND MZGL_LIBS 
		${GLFW_LIBRARIES}
		${OPENGL_gl_LIBRARY}
		winmm
		dsound
		ole32	
	)


elseif(LINUX)
    add_compile_definitions(__LINUX_ALSA__)
    find_package(ALSA REQUIRED)
    
	 # -mfpu=neon-vfpv4
    if(__ARMEL__)
        add_compile_options(-mcpu=cortex-a7 -mfloat-abi=hard -mfpu=neon -O3)

    else(__ARMEL__)
 #   	add_compile_options(-O3)
 #		add_compile_options(-g -O0)
    endif(__ARMEL__)

    #probably want pulseaudio because the latency is supposedly better
       # add_compile_definitions(__LINUX_PULSE__)


       find_package(PkgConfig REQUIRED)
		pkg_ckeck_modules(GTK REQUIRED gtk+-3.0)

		# Setup CMake to use GTK+, tell the compiler where to look for headers
		# and to the linker where to look for libraries
		include_directories(${GTK_INCLUDE_DIRS})
		link_directories(${GTK_LIBRARY_DIRS})

		# Add other flags to the compiler
		add_definitions(${GTK_CFLAGS_OTHER})

		list(APPEND MZGL_LIBS 
			jack 
			atomic # needed for raspberry pi
			GLESv2
			EGL
			${GTK_LIBRARIES}
			${ALSA_LIBRARIES}
		)
endif()


	# windows portaudio: can't seem to get the cmake build working (WASAPI doesn't work)
	# so I built it in visual studio and am attempting to link it here (need to copy dll into bin)
	#include_directories(lib/portaudio/build/msvc/Win32/Release/) ## this line may not be needed
	#add_library(portaudio_x86 SHARED lib/portaudio/build/msvc/x64/Release/portaudio_x86.lib)
	#set_target_properties(portaudio_x86 PROPERTIES LINKER_LANGUAGE C)
#	these 2 apis don't seem to compile with clang-cl for now, can't be bothered to work it out
#	set(PA_USE_WASAPI OFF)
#	set(PA_USE_WDMKS OFF)
if(NOT ANDROID AND NOT WIN32 AND NOT IOS)
	add_subdirectory(lib/portaudio "portaudio-build")
	list(APPEND MZGL_LIBS portaudio_static)
endif()
if(WIN32)
	list(APPEND MZGL_LIBS portaudio_static)
endif()

if(NOT ANDROID AND NOT IOS)
	list(APPEND MZGL_LIBS RtMidi glfw)
endif()

target_link_libraries(libmzgl ${MZGL_LIBS})

if(NOT ANDROID)
	target_link_libraries(libmzgl_unit_test  ${MZGL_LIBS})
endif()
