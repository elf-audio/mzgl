# @brief Recursively add to the search paths
#
# Find all sub dirs of the passed path, and then add its paths to the search
# paths
#
# @param ROOTPATH The source path to start at
function(mzgl_add_search_paths ROOTPATH)

  file(GLOB_RECURSE INCLUDE_DIRECTORIES "${ROOTPATH}/*.h" "${ROOTPATH}/*.hpp")

  foreach(file_path IN LISTS INCLUDE_DIRECTORIES)
    get_filename_component(dir_path "${file_path}" DIRECTORY)
    list(APPEND INCLUDE_DIRECTORIES_LIST "${dir_path}")
  endforeach()

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
