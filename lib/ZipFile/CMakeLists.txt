cmake_minimum_required(VERSION 3.1)
set(CMAKE_CXX_STANDARD 17)  
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

include_directories(
    ../misc
    ../ghc-filesystem/include
    lib/zipper/zipper
    lib/zipper/minizip
    ..
)

target_include_directories(ZipFile PUBLIC
    .
    lib
)
