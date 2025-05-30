#***************************************************************************
# Copyright:  Matthias Schmieder,
# E-Mail:     schmieder.matthias@gmail.com
# Year:       2016
#***************************************************************************
cmake_minimum_required(VERSION 2.8)


# Set a consistent MACOSX_RPATH default across all CMake versions. 
# When CMake 2.8.12 is required, change this default to 1.
# When CMake 3.0.0 is required, remove this block (see CMP0042).
if(NOT DEFINED CMAKE_MACOSX_RPATH)
  set(CMAKE_MACOSX_RPATH 0)
endif()

project("minizip")

# set cmake debug postfix to d
set(CMAKE_DEBUG_POSTFIX "d")
x
message(STATUS "zlib: ${ZLIB}")
message(STATUS "zlib found: ${ZLIB_FOUND}")

if(BUILD_TEST OR BUILD_SHARED_LIBS)
   find_package(ZLIB REQUIRED)
endif()

set(MINIZIP_SRC "ioapi.c"
  "ioapi_buf.c"
  "ioapi_mem.c"
  "unzip.c"
  "zip.c")

set(MINIZIP_PUBLIC_HEADERS "crypt.h"
  "ioapi.h"
  "ioapi_buf.h"
  "ioapi_mem.h"
  "unzip.h"
  "zip.h")

if(WIN32)
  list(APPEND MINIZIP_SRC "iowin32.c")
  list(APPEND MINIZIP_PUBLIC_HEADERS "iowin32.h")
endif()

# create minizip library
add_library(minizip ${MINIZIP_SRC} ${MINIZIP_PUBLIC_HEADERS})

option(USE_AES "enables building of aes library" ON)
if(USE_AES)
  set(AES_SRC 
    aes/aescrypt.c
    aes/aeskey.c
    aes/aestab.c
    aes/entropy.c
    aes/fileenc.c
    aes/hmac.c
    aes/prng.c
    aes/pwd2key.c
    aes/sha1.c)

  set(AES_PUBLIC_HEADERS 
    aes/aes.h
    aes/aes_via_ace.h
    aes/aesopt.h
    aes/aestab.h
    aes/brg_endian.h
    aes/brg_types.h
    aes/entropy.h
    aes/fileenc.h
    aes/hmac.h
    aes/prng.h
    aes/pwd2key.h
    aes/sha1.h)

  add_library(aes ${AES_SRC} ${AES_PUBLIC_HEADERS})

  set_target_properties(aes minizip
     PROPERTIES
     COMPILE_DEFINITIONS "O -DHAVE_AES")

  target_link_libraries(minizip aes)

  install(TARGETS aes EXPORT zlib-exports
    INCLUDES DESTINATION "include"
    RUNTIME DESTINATION  "bin"
    LIBRARY DESTINATION  "lib"
    ARCHIVE DESTINATION  "lib")

  install(FILES ${AES_PUBLIC_HEADERS}
    DESTINATION "include/minizip/aes")
endif()

install(TARGETS minizip EXPORT zlib-exports
  INCLUDES DESTINATION "include"
  RUNTIME DESTINATION "bin"
  LIBRARY DESTINATION "lib"
  ARCHIVE DESTINATION "lib")

install(EXPORT zlib-exports
        DESTINATION "cmake"
        NAMESPACE "MINIZIP::")

install(FILES ${MINIZIP_PUBLIC_HEADERS}
  DESTINATION "include/minizip")

option (BUILD_TEST "enabled building of executables minizip and miniunz. Requires ZLIB!" OFF)
if(BUILD_TEST)
  add_executable(miniunz_exec miniunz.c)
  target_link_libraries(miniunz_exec minizip ZLIB::ZLIB)

  add_executable(minizip_exec minizip.c)
  target_link_libraries(minizip_exec minizip ZLIB::ZLIB)

  install(TARGETS miniunz_exec minizip_exec
          RUNTIME DESTINATION "bin")
endif()

