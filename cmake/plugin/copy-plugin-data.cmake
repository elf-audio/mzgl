# Copy a data/ directory (or a subset of it) into a plugin bundle. Run as a script:
#   cmake -DSRC=<project>/data -DDST=<bundle>/Contents/Resources/data
#         [-DSUBDIRS="font;svg;presets/foo"] -P copy-plugin-data.cmake
#
# With no SUBDIRS the whole of SRC is copied. With SUBDIRS only those
# sub-paths are copied, keeping their relative location under DST
# (presets/foo lands in DST/presets/foo).

if(NOT SRC OR NOT DST)
  message(FATAL_ERROR "copy-plugin-data.cmake: SRC and DST must be set")
endif()

if(NOT SUBDIRS)
  if(EXISTS "${SRC}")
    file(COPY "${SRC}/" DESTINATION "${DST}")
  endif()
  return()
endif()

foreach(sub ${SUBDIRS})
  if(EXISTS "${SRC}/${sub}")
    get_filename_component(parent "${sub}" DIRECTORY)
    file(COPY "${SRC}/${sub}" DESTINATION "${DST}/${parent}")
  endif()
endforeach()
