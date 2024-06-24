if (LINUX)
  add_compile_definitions (__LINUX_ALSA__)
  find_package (ALSA REQUIRED)

  # -mfpu=neon-vfpv4
  if (__ARMEL__)
    add_compile_options (-mcpu=cortex-a7 -mfloat-abi=hard -mfpu=neon -O3)
  endif (__ARMEL__)

  find_package (PkgConfig REQUIRED)
  pkg_check_modules (GTK REQUIRED gtk+-3.0)

  include_directories (${GTK_INCLUDE_DIRS})
  link_directories (${GTK_LIBRARY_DIRS})

  add_definitions (${GTK_CFLAGS_OTHER})

  find_library (JACK_LIB jack)
  find_package (PkgConfig)
  pkg_check_modules (jack jack)

  if (JACK_LIB OR jack_FOUND)
    set (HAVE_JACK TRUE)
  endif ()

  if (NOT HAVE_JACK)
    message (FATAL_ERROR "Jack API requested but no Jack dev libraries found")
  endif ()

  if (NOT jack_FOUND)
    list (APPEND MZGL_LIBRARIES ${JACK_LIB})
  else ()
    list (APPEND MZGL_LIBRARIES ${jack_LINK_LIBRARIES})
  endif ()

  find_package (OpenGL REQUIRED)
  find_package (X11 REQUIRED)
  find_package (glfw3 3.3 REQUIRED)

  if (NOT glfw3_FOUND)
    message (STATUS "GLFW not found, setting manually.")
    set (GLFW3_INCLUDE_DIR "/usr/include/GLFW")
    set (GLFW3_LIBRARY "/usr/lib/x86_64-linux-gnu/libglfw.so")
    include_directories (${GLFW3_INCLUDE_DIR})
    link_directories (${GLFW3_LIBRARY})
  else ()
    include_directories (${glfw3_INCLUDE_DIRS})
    link_directories (${glfw3_LIBRARY_DIRS})
  endif ()

  include_directories (${OPENGL_INCLUDE_DIRS} ${X11_INCLUDE_DIR})

  list (
    APPEND
    MZGL_LIBRARIES
    ${GTK_LIBRARIES}
    ${ALSA_LIBRARIES}
    ${OPENGL_LIBRARIES}
    ${X11_LIBRARIES}
    ${GLFW3_LIBRARY})
endif ()
