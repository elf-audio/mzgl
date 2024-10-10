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
      "c_api")

  foreach(ITEM IN LISTS SUBDIRS)
    if(IS_DIRECTORY ${ITEM})
      # Get the base name of the directory
      get_filename_component(DIRECTORY_NAME ${ITEM} NAME)

      # Filter out directories starting with '.' (both at the root and within
      # subdirectories)
      if(NOT DIRECTORY_NAME MATCHES "^\\."
         AND NOT ITEM MATCHES "/\\.[^/]+/"
         AND NOT DIRECTORY_NAME IN_LIST EXCLUDED_DIRS)

        # Check if the current directory is a subdirectory of an excluded
        # directory
        set(SKIP_DIR FALSE)
        foreach(EXCLUDED_DIR IN LISTS EXCLUDED_DIRS)
          if(ITEM MATCHES "/${EXCLUDED_DIR}/") # Skip subdirectories of excluded
                                               # directories
            set(SKIP_DIR TRUE)
            break()
          endif()
        endforeach()

        # If the directory is not part of an excluded directory's substructure,
        # add it
        if(NOT SKIP_DIR)
          list(APPEND INCLUDE_DIRECTORIES_LIST ${ITEM})
        endif()
      endif()
    endif()
  endforeach()

  list(APPEND INCLUDE_DIRECTORIES_LIST "${ROOTPATH}")
  list(REMOVE_DUPLICATES INCLUDE_DIRECTORIES_LIST)

  include_directories(${INCLUDE_DIRECTORIES_LIST})

  foreach(dir IN LISTS INCLUDE_DIRECTORIES_LIST)
    mzgl_print_verbose_in_yellow("            -> ${dir}")
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

  mzgl_restore_cmake_log_level()
  mzgl_print_verbose_in_yellow(
    "      -> Package name is ${CPM_LAST_PACKAGE_NAME}")

  mzgl_add_search_paths("${${CPM_LAST_PACKAGE_NAME}_SOURCE_DIR}")
endfunction()
