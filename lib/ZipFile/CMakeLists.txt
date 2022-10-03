cmake_minimum_required(VERSION 3.1)
set(CMAKE_CXX_STANDARD 17)  
project(ZipFile)



include_directories(
    ../misc
    ../ghc-filesystem/include
    lib/zipper/zipper
    lib/zipper/minizip
    lib/zipper/
    ..

)


add_library(ZipFile STATIC
    ZipFile.h
    ZipReader.h
    ZipFile.cpp
    ZipStreamer.cpp
    ZipReader.cpp




    lib/zipper/zipper/CDirEntry.cpp
    lib/zipper/zipper/tools.cpp
    lib/zipper/zipper/unzipper.cpp
    lib/zipper/zipper/zipper.cpp

    lib/zipper/minizip/unzip.c
    lib/zipper/minizip/zip.c
    lib/zipper/minizip/ioapi.c
    lib/zipper/minizip/ioapi_buf.c
    lib/zipper/minizip/ioapi_mem.c
    # lib/zipper/zipper/tps/dirent.c
)


target_include_directories(ZipFile PUBLIC
    .
    lib
)

if(APPLE) 



else()

option(BUILD_TEST "" OFF)
add_subdirectory(lib/zipper "zipper-build")

target_include_directories(ZipFile PUBLIC
    .
    lib
)
target_link_libraries(ZipFile Zipper)


endif()