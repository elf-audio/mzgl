cmake_minimum_required(VERSION 3.1)
set(CMAKE_CXX_STANDARD 14)  
project(ZipFile)





add_library(ZipFile STATIC
    ZipFile.h
    ZipReader.h
    ZipFile.cpp
    ZipStreamer.cpp
    ZipReader.cpp
)
option(BUILD_TEST "" OFF)
add_subdirectory(lib/zipper "zipper-build")

target_link_libraries(ZipFile Zipper)
# if(WIN32)
#     list(APPEND ZIPPER_SOURCES lib/zipper/minizip/iowin32.c)
# list(APPEND ZIPPER_SOURCES
#                 lib/zipper/zipper/tps/dirent.c)

#     list(APPEND HEADERS lib/zipper/zipper/tps)
# endif()


# if(WIN32)

#     list(APPEND MZGL_SOURCES
#             lib/glew/src/glew.c
#             #lib/mzgl/app/win64/mainWin.cpp
#             lib/mzgl/util/winUtil.cpp
#             lib/mzgl/util/AlignedAllocator.cpp
#     )
 
include_directories(
    ../misc
    lib/zipper/zipper
    lib/zipper/minizip
    ..
)

target_include_directories(ZipFile PUBLIC
    .
    lib
)
