cmake_minimum_required(VERSION 3.18.1)
set(CMAKE_CXX_STANDARD 20)
project(ZipFile C CXX)

# ZipFile / ZipReader / Gzip are thin wrappers over miniz (lib/miniz, single
# file, MIT). No zlib or other external dependency on any platform.
add_library(
  ZipFile STATIC
  ZipFile.h
  ZipFile.cpp
  ZipReader.h
  ZipReader.cpp
  Gzip.h
  Gzip.cpp
  ZipIO.h
  lib/miniz/miniz.h
  lib/miniz/miniz.c)

target_include_directories(
  ZipFile
  PUBLIC .
  PRIVATE lib/miniz ../misc ..)

# Don't let miniz define zlib-named macros (deflate, crc32, ...) - some TUs in
# the wider project also see the real zlib.h.
target_compile_definitions(ZipFile PRIVATE MINIZ_NO_ZLIB_COMPATIBLE_NAMES)

# Third-party code: silence its warnings (our own sources build with the
# project's normal warning flags).
if(MSVC)
  set_source_files_properties(lib/miniz/miniz.c PROPERTIES COMPILE_OPTIONS "/w")
else()
  set_source_files_properties(lib/miniz/miniz.c PROPERTIES COMPILE_OPTIONS "-w")
endif()

if(NOT WIN32 AND NOT APPLE)
  # 64-bit file offsets for fseeko/ftello on 32-bit Linux / Android so miniz
  # can read archives over 2 GB.
  set_source_files_properties(
    lib/miniz/miniz.c PROPERTIES COMPILE_DEFINITIONS
                                 "_FILE_OFFSET_BITS=64;_LARGEFILE64_SOURCE")
endif()
