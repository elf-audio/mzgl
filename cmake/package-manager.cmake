set(GLOBAL_HEADER_SEARCH_PATHS
    ""
    CACHE STRING "Global header search paths" FORCE)

# @brief Append the cached header search paths
#
# Writes a xcode compatible file to include from
#
# @param PATH the path write
function(mzgl_add_xcode_header_search_path PATH)
  set(CURRENT_LIST "${GLOBAL_HEADER_SEARCH_PATHS}")
  list(APPEND CURRENT_LIST "${PATH}")
  set(GLOBAL_HEADER_SEARCH_PATHS
      "${CURRENT_LIST}"
      CACHE STRING "Global header search paths" FORCE)
endfunction()

function(mzgl_patch_cmake_version CMAKE_LISTS_PATH)
  if(NOT EXISTS "${CMAKE_LISTS_PATH}")
    message(
      WARNING "CMakeLists.txt not found at ${CMAKE_LISTS_PATH}, skipping patch."
    )
    return()
  endif()

  file(READ "${CMAKE_LISTS_PATH}" _cmake_content)

  string(
    REGEX
    REPLACE "cmake_minimum_required\\(VERSION +[0-9]+\\.[0-9]+(\\.[0-9]+)?\\)"
            "cmake_minimum_required(VERSION 3.5...${CMAKE_VERSION})"
            _cmake_content "${_cmake_content}")

  string(REGEX
         REPLACE "cmake_policy\\( *VERSION +[0-9]+\\.[0-9]+(\\.[0-9]+)? *\\)"
                 "cmake_policy(VERSION 3.5)" _cmake_content "${_cmake_content}")

  file(WRITE "${CMAKE_LISTS_PATH}" "${_cmake_content}")
  message(
    STATUS
      "Patched ${CMAKE_LISTS_PATH} to use minimum CMake version 3.5 for compatibility."
  )
endfunction()

# @brief Write the include paths to a file in cpm-source-cache
#
# Writes a xcode compatible file to include from
#
# @param PATH the path write
function(mzgl_write_header_search_paths)
  set(CURRENT_LIST "${GLOBAL_HEADER_SEARCH_PATHS}")
  list(REMOVE_DUPLICATES CURRENT_LIST)
  file(WRITE "${XCODE_HEADER_PATH_FILE}" "")
  foreach(HEADER_SEARCH_PATH IN LISTS CURRENT_LIST)
    file(APPEND "${XCODE_HEADER_PATH_FILE}"
         "HEADER_SEARCH_PATHS = $(inherited) ${HEADER_SEARCH_PATH}\n")
  endforeach()
endfunction()

# @brief Recursively add to the search paths
#
# Find all sub dirs of the passed path, and then add its paths to the search
# paths
#
# @param ROOTPATH The source path to start at
function(mzgl_add_search_paths ROOTPATH)
  set(INCLUDE_DIRECTORIES_LIST)
  file(
    GLOB_RECURSE SUBDIRS
    LIST_DIRECTORIES true
    "${ROOTPATH}/*")

  set(EXCLUDED_DIRS
      "tests"
      "benchmarks"
      "README"
      "LICENSE"
      "VERSION"
      "makefile"
      "LICENSES"
      "cmake"
      "docs"
      "build"
      "c_api"
      "example"
      "screenshots"
      "testbed")

  foreach(ITEM IN LISTS SUBDIRS)
    if(IS_DIRECTORY ${ITEM})
      get_filename_component(DIRECTORY_NAME ${ITEM} NAME)
      if(NOT DIRECTORY_NAME MATCHES "^\\."
         AND NOT ITEM MATCHES "/\\.[^/]+/"
         AND NOT DIRECTORY_NAME IN_LIST EXCLUDED_DIRS)
        set(SKIP_DIR FALSE)
        foreach(EXCLUDED_DIR IN LISTS EXCLUDED_DIRS)
          if(ITEM MATCHES "/${EXCLUDED_DIR}/")
            set(SKIP_DIR TRUE)
            break()
          endif()
        endforeach()
        if(NOT SKIP_DIR)
          list(APPEND INCLUDE_DIRECTORIES_LIST ${ITEM})
        endif()
      endif()
    endif()
  endforeach()

  list(APPEND INCLUDE_DIRECTORIES_LIST "${ROOTPATH}")
  list(REMOVE_DUPLICATES INCLUDE_DIRECTORIES_LIST)

  include_directories(${INCLUDE_DIRECTORIES_LIST})

  foreach(DIR IN LISTS INCLUDE_DIRECTORIES_LIST)
    mzgl_print_debug_in_grey("            -> ${DIR}")
    mzgl_add_xcode_header_search_path(${DIR})
  endforeach()
endfunction()

# @brief Add a new package to the project
#
# Long description here with more details.
#
# @param PACKAGE_NAME The name of the package to add. See the CPM documentation
# for more info on this but ideally, you want to use the shorthand gh:/ type
# path
function(mzgl_add_package PACKAGE_NAME)
  mzgl_print_in_yellow("[CPM] -> Adding package ${PACKAGE_NAME}")
  mzgl_save_cmake_log_level()

  cpmaddpackage(${PACKAGE_NAME})

  set("${CPM_LAST_PACKAGE_NAME}_DEPS_DIR"
      "${${CPM_LAST_PACKAGE_NAME}_SOURCE_DIR}"
      CACHE INTERNAL "${CPM_LAST_PACKAGE_NAME} Deps dir")

  mzgl_patch_cmake_version(
    "${${CPM_LAST_PACKAGE_NAME}_SOURCE_DIR}/CMakeLists.txt")

  mzgl_restore_cmake_log_level()
  mzgl_print_debug_in_grey("      -> Package name is ${CPM_LAST_PACKAGE_NAME}")

  mzgl_add_search_paths("${${CPM_LAST_PACKAGE_NAME}_SOURCE_DIR}")
endfunction()

function(mzgl_add_named_package PACKAGE_STRING)
  string(REGEX REPLACE "^gh:" "" PACKAGE_STRING "${PACKAGE_STRING}")
  string(REGEX MATCH "^[^/]+" DEVELOPER "${PACKAGE_STRING}")
  string(REGEX REPLACE "^[^/]+/" "" PACKAGE_STRING "${PACKAGE_STRING}")
  string(REGEX MATCH "^[^#]+" NAME "${PACKAGE_STRING}")
  string(REGEX REPLACE "^[^#]+#" "" SHA "${PACKAGE_STRING}")

  set(TARGET ${NAME})

  mzgl_print_debug_in_grey("Named package -> Developer: ${DEVELOPER}")
  mzgl_print_debug_in_grey("Named package -> Name: ${NAME}")
  mzgl_print_debug_in_grey("Named package -> SHA: ${SHA}")
  mzgl_print_debug_in_grey("Named package -> Target: ${TARGET}")

  mzgl_print_in_yellow("[CPM] -> Adding package gh:${DEVELOPER}/${NAME}#${SHA}")

  mzgl_save_cmake_log_level()

  cpmaddpackage(
    NAME ${TARGET} URL
    https://github.com/${DEVELOPER}/${NAME}/archive/${SHA}.zip DOWNLOAD_ONLY
    True)

  add_library(${TARGET} INTERFACE IMPORTED GLOBAL)
  target_include_directories(${TARGET} SYSTEM INTERFACE ${${TARGET}_SOURCE_DIR})
  include_directories(SYSTEM INTERFACE ${${TARGET}_SOURCE_DIR})

  set(ENV{${TARGET}_SOURCE_DIR} "${TARGET}_SOURCE_DIR}")
  set("${CPM_LAST_PACKAGE_NAME}_DEPS_DIR"
      "${${CPM_LAST_PACKAGE_NAME}_SOURCE_DIR}"
      CACHE INTERNAL "${CPM_LAST_PACKAGE_NAME} Deps dir")
  mzgl_restore_cmake_log_level()
  mzgl_print_debug_in_grey("      -> Package name is ${CPM_LAST_PACKAGE_NAME}")
endfunction()
