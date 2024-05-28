cmake_minimum_required(VERSION 3.18 FATAL_ERROR)

project(zip-file)

add_library(zip-file STATIC
            ZipFile.h
            ZipReader.h
            ZipFile.cpp
            ZipStreamer.cpp
            ZipReader.cpp)
mzgl_set_target_properties(zip-file)
target_include_directories(zip-file PUBLIC ../)
target_link_libraries(zip-file PUBLIC staticZipper fsystem ghcfilesystem)