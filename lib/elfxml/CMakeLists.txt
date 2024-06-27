cmake_minimum_required (VERSION 3.18 FATAL_ERROR)

project (elfxml)

add_library (elfxml STATIC elfxml.h elfxml.cpp)

function (elfxml_replace_namespace EXTENSION)
  mzgl_print_verbose_in_red ("[Elf-Xml] -> Updating pugixml namespace in ${EXTENSION}")
  file (READ ${pugixml_SOURCE_DIR}/src/pugixml.${EXTENSION} FILE_CONTENTS)
  string (REPLACE "namespace pugi" "namespace elfxml::pugi" FILE_CONTENTS "${FILE_CONTENTS}")
  if (WIN32)
    string (REPLACE "const pugi::" "const elfxml::pugi::" FILE_CONTENTS "${FILE_CONTENTS}")
  endif()
  file (WRITE ${pugixml_SOURCE_DIR}/src/pugixml.${EXTENSION} "${FILE_CONTENTS}")
endfunction ()

elfxml_replace_namespace ("hpp")
elfxml_replace_namespace ("cpp")

target_include_directories (elfxml PUBLIC ../)
target_include_directories (elfxml PUBLIC ${pugixml_SOURCE_DIR}/src)
