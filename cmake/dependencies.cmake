function(mzgl_add_packages)
  mzgl_add_package("gh:nlohmann/json@3.11.3")
  mzgl_add_package("gh:cameron314/concurrentqueue#00dd7ba")
  mzgl_add_package("gh:cameron314/readerwriterqueue@1.0.7")
  mzgl_add_package("gh:elf-audio/fontstash#357bef7")
  mzgl_add_package("gh:elf-audio/fast-poly2tri#5dcc516")

  # Packages without CMake targets need manual include dirs.
  include_directories("${fontstash_DEPS_DIR}/src")
  include_directories("${fast-poly2tri_DEPS_DIR}")
  mzgl_add_xcode_header_search_path("${fontstash_DEPS_DIR}/src")
  mzgl_add_xcode_header_search_path("${fast-poly2tri_DEPS_DIR}")
endfunction()
