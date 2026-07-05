# Compile-time graphics backend selection - the single source of truth.
#
# Included by mzgl's own CMakeLists.txt and by parent projects (before their
# add_subdirectory(mzgl)) so both resolve identical settings. Idempotent.
#
#   MZGL_GRAPHICS_BACKEND = OpenGL | Sokol | Metal
#
#   OpenGL - GL 3.2+ / GLES3 backend (all platforms)
#   Sokol  - sokol_gfx: Metal on Apple, D3D11 on Windows, GLES3 elsewhere
#   Metal  - native Metal backend (Apple only); consumes the same
#            sokol-shdc-compiled shaders as the Sokol backend
#
# Defaults: macOS/iOS -> Metal, Windows -> Sokol (D3D11), everything else
# (Android/Linux) -> OpenGL.
#
# Outputs (plain variables in the including scope and below):
#   USING_SOKOL / USING_METAL / USING_OPENGL   - exactly one is ON
#   MZGL_BACKEND_COMPILE_DEFS                  - pass to add_compile_definitions()
#
# Back-compat: parents that only set USING_SOKOL get Sokol/OpenGL as before.

if(DEFINED MZGL_GRAPHICS_BACKEND_RESOLVED)
  return()
endif()
set(MZGL_GRAPHICS_BACKEND_RESOLVED TRUE)

if(NOT DEFINED MZGL_GRAPHICS_BACKEND OR MZGL_GRAPHICS_BACKEND STREQUAL "")
  if(DEFINED USING_SOKOL)
    # legacy flag (also lingers in old build dirs' caches): honour it
    if(USING_SOKOL)
      set(_mzgl_backend_default "Sokol")
    else()
      set(_mzgl_backend_default "OpenGL")
    endif()
  elseif(APPLE)
    set(_mzgl_backend_default "Metal")
  elseif(WIN32)
    set(_mzgl_backend_default "Sokol")
  else()
    set(_mzgl_backend_default "OpenGL")
  endif()
  set(MZGL_GRAPHICS_BACKEND "${_mzgl_backend_default}")
endif()

set(MZGL_GRAPHICS_BACKEND
    "${MZGL_GRAPHICS_BACKEND}"
    CACHE STRING "Graphics backend: OpenGL, Sokol or Metal (Apple only)")
set_property(CACHE MZGL_GRAPHICS_BACKEND PROPERTY STRINGS "OpenGL" "Sokol"
                                                  "Metal")

string(TOLOWER "${MZGL_GRAPHICS_BACKEND}" _mzgl_backend_lower)

set(USING_SOKOL OFF)
set(USING_METAL OFF)
set(USING_OPENGL OFF)

# MZGL_COMPILED_SHADERS marks the backends that consume sokol-shdc-compiled
# shaders (registered by name) rather than loading GLSL files at runtime.
if(_mzgl_backend_lower STREQUAL "sokol")
  set(USING_SOKOL ON)
  set(MZGL_BACKEND_COMPILE_DEFS MZGL_SOKOL MZGL_COMPILED_SHADERS)
  if(APPLE)
    list(APPEND MZGL_BACKEND_COMPILE_DEFS SOKOL_METAL)
  elseif(WIN32)
    list(APPEND MZGL_BACKEND_COMPILE_DEFS SOKOL_D3D11)
  endif()
elseif(_mzgl_backend_lower STREQUAL "metal")
  if(NOT APPLE)
    message(
      FATAL_ERROR
        "MZGL_GRAPHICS_BACKEND=Metal is only available on Apple platforms")
  endif()
  set(USING_METAL ON)
  set(MZGL_BACKEND_COMPILE_DEFS MZGL_METAL MZGL_COMPILED_SHADERS)
elseif(_mzgl_backend_lower STREQUAL "opengl")
  set(USING_OPENGL ON)
  set(MZGL_BACKEND_COMPILE_DEFS MZGL_OPENGL)
else()
  message(
    FATAL_ERROR
      "Unknown MZGL_GRAPHICS_BACKEND '${MZGL_GRAPHICS_BACKEND}' - expected OpenGL, Sokol or Metal"
  )
endif()

# The cached MZGL_GRAPHICS_BACKEND wins over the legacy USING_SOKOL flag on
# reconfigures of an existing build dir - warn if they contradict so a
# `-DUSING_SOKOL=...` that is being ignored doesn't go unnoticed.
if(DEFINED CACHE{USING_SOKOL})
  set(_mzgl_legacy_sokol "$CACHE{USING_SOKOL}")
  if((_mzgl_legacy_sokol AND NOT USING_SOKOL) OR (NOT _mzgl_legacy_sokol
                                                  AND USING_SOKOL))
    message(
      WARNING
        "USING_SOKOL=$CACHE{USING_SOKOL} is ignored: the graphics backend is "
        "already resolved to '${MZGL_GRAPHICS_BACKEND}'. Reconfigure with "
        "-DMZGL_GRAPHICS_BACKEND=<OpenGL|Sokol|Metal> to change it.")
  endif()
endif()

message(STATUS "mzgl graphics backend: ${MZGL_GRAPHICS_BACKEND}")
