if (LINUX)
  add_compile_definitions (__LINUX_ALSA__)

  # -mfpu=neon-vfpv4
  if (__ARMEL__)
    add_compile_options (-mcpu=cortex-a7 -mfloat-abi=hard -mfpu=neon -O3)
  endif (__ARMEL__)

  find_package (ALSA REQUIRED)
  find_package (PkgConfig REQUIRED)
  find_library (JACK_LIB jack)
  find_package (PkgConfig)
  find_package (OpenGL REQUIRED)
  find_package (X11 REQUIRED)

  pkg_check_modules (GLFW REQUIRED glfw3)
  pkg_check_modules (GTK REQUIRED gtk+-3.0)
  pkg_check_modules (jack jack)

  include_directories (${GTK_INCLUDE_DIRS})
  link_directories (${GTK_LIBRARY_DIRS})

  add_definitions (${GTK_CFLAGS_OTHER})

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

  include_directories (${OPENGL_INCLUDE_DIRS} ${X11_INCLUDE_DIR} ${GLFW_INCLUDE_DIRS} "/usr/include")
  link_directories (${GTK_LIBRARY_DIRS} ${GLFW_LIBRARY_DIRS} "/usr/lib/x86_64-linux-gnu")

  list (
    APPEND
    MZGL_LIBRARIES
    ${GTK_LIBRARIES}
    ${ALSA_LIBRARIES}
    ${OPENGL_LIBRARIES}
    ${X11_LIBRARIES}
    ${GLFW3_LIBRARY}
    glfw)
endif ()
