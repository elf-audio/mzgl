cmake_minimum_required(VERSION 3.1)
set(CMAKE_CXX_STANDARD 17)
project(ZipFile)



include_directories(
    ../misc
    ../ghc-filesystem/include
    lib/zipper/zipper
    lib/zipper/minizip
    lib/zipper/
    lib/zipper/utf8helper/
#    lib/zipper/zlib
    ..
)

if(WIN32)
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
    # lib/zipper/minizip/iowin32.c
    # lib/zipper/zipper/tps/dirent.c

    lib/zipper/zlib/adler32.c
    lib/zipper/zlib/compress.c
    lib/zipper/zlib/crc32.c
    lib/zipper/zlib/deflate.c
    lib/zipper/zlib/gzclose.c
    lib/zipper/zlib/gzlib.c
    lib/zipper/zlib/gzread.c
    lib/zipper/zlib/gzwrite.c
    lib/zipper/zlib/infback.c
    lib/zipper/zlib/inffast.c
    lib/zipper/zlib/inflate.c
    lib/zipper/zlib/inftrees.c
    lib/zipper/zlib/trees.c
    lib/zipper/zlib/uncompr.c
    lib/zipper/zlib/zutil.c

    lib/zipper/utf8helper/win32io_utf8.cpp
)
else()
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

endif()

target_include_directories(ZipFile PUBLIC
    .
    lib
)

if(WIN32)

target_include_directories(ZipFile PUBLIC lib/zipper/zlib)

else()

option(BUILD_TEST "" OFF)
add_subdirectory(lib/zipper "zipper-build")

target_include_directories(ZipFile PUBLIC
    .
    lib
)
target_link_libraries(ZipFile staticZipper)


endif()