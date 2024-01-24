if(LINUX)
    add_compile_definitions(__LINUX_ALSA__)
    find_package(ALSA REQUIRED)

    # -mfpu=neon-vfpv4
    if(__ARMEL__)
        add_compile_options(-mcpu=cortex-a7 -mfloat-abi=hard -mfpu=neon -O3)

    else(__ARMEL__)
        # add_compile_options(-O3)
        # add_compile_options(-g -O0)
    endif(__ARMEL__)

    # probably want pulseaudio because the latency is supposedly better
    # add_compile_definitions(__LINUX_PULSE__)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GTK REQUIRED gtk+-3.0)

    # Setup CMake to use GTK+, tell the compiler where to look for headers
    # and to the linker where to look for libraries
    include_directories(${GTK_INCLUDE_DIRS})
    link_directories(${GTK_LIBRARY_DIRS})

    # Add other flags to the compiler
    add_definitions(${GTK_CFLAGS_OTHER})

    # Check for Jack (any OS)
    find_library(JACK_LIB jack)
    find_package(PkgConfig)
    pkg_check_modules(jack jack)

    if(JACK_LIB OR jack_FOUND)
        set(HAVE_JACK TRUE)
    endif()

    if(NOT HAVE_JACK)
        message(FATAL_ERROR "Jack API requested but no Jack dev libraries found")
    endif()

    if(NOT jack_FOUND)
        list(APPEND MZGL_LIBRARIES ${JACK_LIB})
    else()
        list(APPEND MZGL_LIBRARIES ${jack_LINK_LIBRARIES})
    endif()

    list(APPEND MZGL_LIBRARIES
        ${GTK_LIBRARIES}
        ${ALSA_LIBRARIES}
    )
endif()