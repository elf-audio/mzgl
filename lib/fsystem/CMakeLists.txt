cmake_minimum_required (VERSION 3.18 FATAL_ERROR)

project (fsystem)

add_library (fsystem INTERFACE)
target_include_directories (fsystem INTERFACE ../)
target_link_libraries (fsystem INTERFACE ghcfilesystem)