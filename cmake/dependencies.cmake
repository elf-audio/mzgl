function(mzgl_add_packages)
  mzgl_add_package("gh:nlohmann/json@3.11.3")
  mzgl_add_package("gh:cameron314/concurrentqueue#00dd7ba")
  mzgl_add_package("gh:cameron314/readerwriterqueue@1.0.7")
  mzgl_add_package("gh:elf-audio/fontstash#357bef7")

  # The triangulator (SvgDocument, Graphics::drawShape), unmodified upstream. Sources only: it has no
  # CMake build, the tess2 target is in CMakeLists.txt.
  mzgl_add_named_package("gh:memononen/libtess2#8dbd6483e920311a58c9af10a10beb278efebc36")

  # Packages without CMake targets need manual include dirs.
  include_directories("${fontstash_DEPS_DIR}/src")
  include_directories(SYSTEM "${libtess2_DEPS_DIR}/Include")

  # Packages with CMake targets still need Xcode header search paths
  # since Xcode's native build doesn't use CMake's target system.
endfunction()
