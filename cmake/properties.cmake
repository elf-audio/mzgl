function (mzgl_set_target_properties TARGET)
  set_target_properties (${TARGET} PROPERTIES CXX_STANDARD 17 CXX_EXTENSIONS OFF)

  if (APPLE)
    target_compile_definitions (${TARGET} PUBLIC $<IF:$<CONFIG:Debug>,_DEBUG=1,_NDEBUG=1>
                                                 $<IF:$<CONFIG:Debug>,DEBUG=1,NDEBUG=1>)

    target_compile_options (
      ${TARGET}
      PRIVATE # -Wall -Wextra -Wshadow -Werror -Wreorder -Wstring-conversion -Wimplicit-int-float-conversion
              # -Wnon-virtual-dtor -Wunused-private-field
              $<IF:$<CONFIG:Debug>,-DDEBUG,-DNDEBUG> $<$<CONFIG:Release>:-DRELEASE>)

  elseif (MSVC)
    target_compile_options (${TARGET} PRIVATE $<IF:$<CONFIG:Debug>,/Od,/Ox> $<$<CONFIG:Release>:/O2> /W4 /MP)

    target_link_options (${TARGET} PRIVATE $<$<CONFIG:Release>:/MAP> $<$<CONFIG:Release>:/DEBUG:FULL>)
  endif ()
endfunction ()

function (mzgl_compile_as_objc FILE)
  if (APPLE)
    set_source_files_properties (${FILE} PROPERTIES COMPILE_FLAGS "-x objective-c++")
  endif ()
endfunction ()
