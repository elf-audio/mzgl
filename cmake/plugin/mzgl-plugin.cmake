# mzgl plugin build helpers - wrap an mzgl ::Plugin + PluginEditor as a
# VST3 (.vst3), an AUv2 (.component) and/or an AUv3 (.appex inside a container
# .app), all sharing the same plugin/editor code. See mzgl/examples/05_Plugin
# for a complete minimal example.
#
#   include(${MZGL_ROOT}/cmake/plugin/mzgl-plugin.cmake)
#   mzgl_fetch_vst3_sdk()        # once; sets VST3_SDK_ROOT
#   mzgl_fetch_auv2_sdk()        # once; builds `ausdk`, sets AUV2_SDK_ROOT
#
#   mzgl_add_vst3_plugin(<target> ...)   any generator, macOS/Windows/Linux
#   mzgl_add_auv2_plugin(<target> ...)   any generator, macOS only
#   mzgl_add_auv3_plugin(<target> ...)   Xcode generator only, macOS only
#
# The lower-level mzgl_vst3_configure_bundle() / mzgl_auv2_configure_bundle()
# do everything except add_library(), for projects that assemble their own
# source list (a whole app compiled into the plugin, extra libraries, ...).
#
# Requirements: libmzgl must be a target (add_subdirectory(mzgl)). The backend
# compile definitions (MZGL_METAL / MZGL_SOKOL / MZGL_OPENGL) are applied to
# each plugin target here, so a parent project doesn't have to.

set(MZGL_PLUGIN_CMAKE_DIR ${CMAKE_CURRENT_LIST_DIR})
get_filename_component(MZGL_PLUGIN_MZGL_ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
set(MZGL_PLUGIN_SRC_DIR ${MZGL_PLUGIN_MZGL_ROOT}/lib/mzgl/plugin)
set(MZGL_PLUGIN_RESOURCE_DIR ${MZGL_PLUGIN_CMAKE_DIR}/resource)

include(${MZGL_PLUGIN_CMAKE_DIR}/vst3-sdk.cmake)
include(${MZGL_PLUGIN_CMAKE_DIR}/auv2-sdk.cmake)
include(${MZGL_PLUGIN_MZGL_ROOT}/cmake/GraphicsBackend.cmake)

# ---------------------------------------------------------------------------
# shared bits
# ---------------------------------------------------------------------------

# AudioComponents.version is a packed 0xMMmmpp integer (1.6.0 -> 0x10600).
function(mzgl_au_packed_version out_var version)
  string(REPLACE "." ";" _parts "${version}")
  list(LENGTH _parts _len)
  if(NOT _len EQUAL 3)
    message(FATAL_ERROR "mzgl plugin: VERSION must be major.minor.patch, got '${version}'")
  endif()
  list(GET _parts 0 _major)
  list(GET _parts 1 _minor)
  list(GET _parts 2 _patch)
  math(EXPR _packed "${_major} * 65536 + ${_minor} * 256 + ${_patch}")
  set(${out_var} ${_packed} PARENT_SCOPE)
endfunction()

# <tag> entries for an AudioComponents "tags" array.
function(mzgl_au_tags_xml out_var)
  set(_xml "")
  foreach(tag ${ARGN})
    string(APPEND _xml "\t\t\t\t<string>${tag}</string>\n")
  endforeach()
  set(${out_var} "${_xml}" PARENT_SCOPE)
endfunction()

# POST_BUILD: copy DATA_DIR (or DATA_SUBDIRS of it) into <bundle>/Contents/Resources/data,
# then re-sign ad hoc. The re-sign must come AFTER every resource is in place,
# otherwise macOS refuses to load the bundle ("code has no resources but signature
# indicates they must be present") - the linker-time signature was sealed before
# PkgInfo + data/ were copied in.
function(_mzgl_plugin_bundle_post_build target data_dir data_subdirs)
  file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/${target}-PkgInfo "BNDL????")
  add_custom_command(
    TARGET ${target} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/${target}-PkgInfo
            $<TARGET_BUNDLE_CONTENT_DIR:${target}>/PkgInfo)
  if(data_dir)
    string(REPLACE ";" "\;" _subdirs "${data_subdirs}")
    add_custom_command(
      TARGET ${target} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_BUNDLE_CONTENT_DIR:${target}>/Resources/data
      COMMAND ${CMAKE_COMMAND} -DSRC=${data_dir}
              -DDST=$<TARGET_BUNDLE_CONTENT_DIR:${target}>/Resources/data
              "-DSUBDIRS=${_subdirs}" -P ${MZGL_PLUGIN_CMAKE_DIR}/copy-plugin-data.cmake
      COMMENT "[${target}] bundling data/")
  endif()
  if(APPLE)
    add_custom_command(
      TARGET ${target} POST_BUILD
      COMMAND codesign --force --sign - --timestamp=none --deep $<TARGET_BUNDLE_DIR:${target}>
      COMMENT "[${target}] re-signing (ad hoc)")
  endif()
endfunction()

# ---------------------------------------------------------------------------
# VST3
# ---------------------------------------------------------------------------

# The generic single-component wrapper + Cocoa view. Pass VIEW_ONLY to get just
# the view (for a plugin that brings its own SingleComponentEffect).
# The editor view is Cocoa (macOS) for now; on Windows/Linux the DSP side builds
# but createView() has nothing to return until an IPlugView for those hosts exists.
function(mzgl_vst3_wrapper_sources out_var)
  cmake_parse_arguments(ARG "VIEW_ONLY" "" "" ${ARGN})
  set(_srcs ${MZGL_PLUGIN_SRC_DIR}/vst3/MzglVST3View.h)
  if(APPLE)
    list(APPEND _srcs ${MZGL_PLUGIN_SRC_DIR}/vst3/MzglVST3View.mm)
  endif()
  if(NOT ARG_VIEW_ONLY)
    list(APPEND _srcs
         ${MZGL_PLUGIN_SRC_DIR}/vst3/MzglVST3SingleComponent.h
         ${MZGL_PLUGIN_SRC_DIR}/vst3/MzglVST3SingleComponent.cpp
         ${MZGL_PLUGIN_SRC_DIR}/vst3/MzglVST3MidiCC.h
         ${VST3_SDK_ROOT}/public.sdk/source/vst/vstsinglecomponenteffect.cpp)
  endif()
  set(${out_var} ${_srcs} PARENT_SCOPE)
endfunction()

# mzgl_vst3_configure_bundle(<target>
#   BUNDLE_ID <id> VERSION <x.y.z[.build]>
#   [PRODUCT_NAME <name>]        bundle/file name (default: target)
#   [COPYRIGHT <string>] [INFO_PLIST <file.in>]
#   [DATA_DIR <dir> [DATA_SUBDIRS <sub>...]]
#   [OBJCXX_SOURCES <file>...]   .cpp files to compile as ObjC++/ARC on Apple)
#
# <target> must already be a MODULE library holding the plugin + wrapper sources.
# Adds the SDK's platform entry point, includes/defines/links for the VST3 SDK,
# the bundle layout, PkgInfo, data/ and an install() rule into the user's
# plug-in folder.
function(mzgl_vst3_configure_bundle target)
  cmake_parse_arguments(ARG "" "PRODUCT_NAME;BUNDLE_ID;VERSION;COPYRIGHT;INFO_PLIST;DATA_DIR"
                        "DATA_SUBDIRS;OBJCXX_SOURCES" ${ARGN})
  if(NOT ARG_BUNDLE_ID OR NOT ARG_VERSION)
    message(FATAL_ERROR "mzgl_vst3_configure_bundle: BUNDLE_ID and VERSION are required")
  endif()
  if(NOT VST3_SDK_ROOT)
    message(FATAL_ERROR "mzgl_vst3_configure_bundle: call mzgl_fetch_vst3_sdk() first")
  endif()
  if(NOT ARG_PRODUCT_NAME)
    set(ARG_PRODUCT_NAME ${target})
  endif()
  if(NOT ARG_COPYRIGHT)
    set(ARG_COPYRIGHT "")
  endif()
  if(NOT ARG_INFO_PLIST)
    set(ARG_INFO_PLIST ${MZGL_PLUGIN_RESOURCE_DIR}/Info-vst3.plist.in)
  endif()

  if(APPLE)
    target_sources(${target} PRIVATE ${VST3_SDK_ROOT}/public.sdk/source/main/macmain.cpp)
  elseif(WIN32)
    target_sources(${target} PRIVATE ${VST3_SDK_ROOT}/public.sdk/source/main/dllmain.cpp)
  else()
    target_sources(${target} PRIVATE ${VST3_SDK_ROOT}/public.sdk/source/main/linuxmain.cpp)
  endif()

  target_compile_definitions(${target} PRIVATE MZGL_PLUGIN_VST ${MZGL_BACKEND_COMPILE_DEFS}
                                               $<$<CXX_COMPILER_ID:MSVC>:NOMINMAX>)
  target_include_directories(${target} PRIVATE ${MZGL_PLUGIN_SRC_DIR}/vst3 ${VST3_SDK_ROOT})
  target_link_libraries(${target} PRIVATE sdk sdk_common base pluginterfaces)

  if(APPLE)
    target_link_libraries(${target} PRIVATE "-framework Cocoa")
    target_link_options(${target} PRIVATE
      "LINKER:-exported_symbols_list,${VST3_SDK_ROOT}/public.sdk/source/main/macexport.exp")
    set_property(TARGET ${target} APPEND PROPERTY LINK_DEPENDS
                 ${VST3_SDK_ROOT}/public.sdk/source/main/macexport.exp)

    set_target_properties(${target} PROPERTIES
      BUNDLE TRUE
      BUNDLE_EXTENSION vst3
      SUFFIX .vst3
      OUTPUT_NAME "${ARG_PRODUCT_NAME}"
      XCODE_ATTRIBUTE_WRAPPER_EXTENSION vst3
      XCODE_ATTRIBUTE_PRODUCT_NAME "${ARG_PRODUCT_NAME}"
      MACOSX_BUNDLE_GUI_IDENTIFIER ${ARG_BUNDLE_ID}
      MACOSX_BUNDLE_BUNDLE_NAME "${ARG_PRODUCT_NAME}"
      MACOSX_BUNDLE_BUNDLE_VERSION "${ARG_VERSION}"
      MACOSX_BUNDLE_SHORT_VERSION_STRING "${ARG_VERSION}"
      MACOSX_BUNDLE_COPYRIGHT "${ARG_COPYRIGHT}"
      MACOSX_BUNDLE_INFO_PLIST ${ARG_INFO_PLIST})

    _mzgl_plugin_bundle_post_build(${target} "${ARG_DATA_DIR}" "${ARG_DATA_SUBDIRS}")
    install(TARGETS ${target} DESTINATION "$ENV{HOME}/Library/Audio/Plug-Ins/VST3")

    # set_source_files_properties is directory-scoped; this runs in the caller's
    # directory, which is where the plugin's CMakeLists lives.
    set_source_files_properties(${MZGL_PLUGIN_SRC_DIR}/vst3/MzglVST3View.mm ${ARG_OBJCXX_SOURCES}
                                PROPERTIES COMPILE_FLAGS "-x objective-c++ -fobjc-arc")
  elseif(WIN32)
    set_target_properties(${target} PROPERTIES OUTPUT_NAME "${ARG_PRODUCT_NAME}" SUFFIX .vst3)
    install(TARGETS ${target} DESTINATION "$ENV{PROGRAMFILES}/Common Files/VST3")
  else()
    set_target_properties(${target} PROPERTIES OUTPUT_NAME "${ARG_PRODUCT_NAME}" SUFFIX .so PREFIX "")
    install(TARGETS ${target} DESTINATION "$ENV{HOME}/.vst3")
  endif()
endfunction()

# mzgl_add_vst3_plugin(<target>
#   BUNDLE_ID <id> VERSION <x.y.z>
#   SOURCES <plugin sources>          must include a file with the VST3 factory
#                                     (BEGIN_FACTORY_DEF / DEF_CLASS2) and an
#                                     MzglVST3SingleComponent subclass
#   [PRODUCT_NAME <name>] [COPYRIGHT <s>] [INFO_PLIST <f>]
#   [OBJCXX_SOURCES <f>...] [EXTRA_DEFINITIONS <d>...] [LINK_LIBRARIES <lib>...]
#   [DATA_DIR <dir> [DATA_SUBDIRS <sub>...]])
function(mzgl_add_vst3_plugin target)
  cmake_parse_arguments(ARG "" "PRODUCT_NAME;BUNDLE_ID;VERSION;COPYRIGHT;INFO_PLIST;DATA_DIR"
                        "SOURCES;OBJCXX_SOURCES;EXTRA_DEFINITIONS;LINK_LIBRARIES;DATA_SUBDIRS" ${ARGN})
  mzgl_vst3_wrapper_sources(_wrapper)
  add_library(${target} MODULE ${ARG_SOURCES} ${_wrapper})
  target_link_libraries(${target} PRIVATE libmzgl ${ARG_LINK_LIBRARIES})
  target_compile_definitions(${target} PRIVATE ${ARG_EXTRA_DEFINITIONS})
  set(_fwd)
  foreach(k PRODUCT_NAME BUNDLE_ID VERSION COPYRIGHT INFO_PLIST DATA_DIR)
    if(ARG_${k})
      list(APPEND _fwd ${k} "${ARG_${k}}")
    endif()
  endforeach()
  if(ARG_DATA_SUBDIRS)
    list(APPEND _fwd DATA_SUBDIRS ${ARG_DATA_SUBDIRS})
  endif()
  if(ARG_OBJCXX_SOURCES)
    list(APPEND _fwd OBJCXX_SOURCES ${ARG_OBJCXX_SOURCES})
  endif()
  mzgl_vst3_configure_bundle(${target} ${_fwd})
endfunction()

# ---------------------------------------------------------------------------
# AUv2
# ---------------------------------------------------------------------------

function(mzgl_auv2_wrapper_sources out_var)
  set(${out_var}
      ${MZGL_PLUGIN_SRC_DIR}/auv2/MzglAUv2.h
      ${MZGL_PLUGIN_SRC_DIR}/auv2/MzglAUv2.cpp
      ${MZGL_PLUGIN_SRC_DIR}/auv2/MzglAUv2View.h
      ${MZGL_PLUGIN_SRC_DIR}/auv2/MzglAUv2View.mm
      PARENT_SCOPE)
endfunction()

# mzgl_auv2_configure_bundle(<target>
#   PRODUCT_NAME <name> BUNDLE_ID <id> VERSION <x.y.z>
#   AU_TYPE <aufx|aumf|aumu> AU_SUBTYPE <4cc> AU_MANUFACTURER <4cc>
#   AU_NAME "<Manufacturer: Product>" FACTORY_FUNCTION <sym>
#   [AU_DESCRIPTION <s>] [AU_TAGS <tag>...] [COPYRIGHT <s>] [INFO_PLIST <f>]
#   [AU_VERSION_INT <n>]         AudioComponents.version; default packed from a
#                                major.minor.patch VERSION (required otherwise)
#   [EXTRA_FACTORY_FUNCTIONS <sym>...]  further exported entry points, for a
#                                bundle whose INFO_PLIST publishes several
#                                AudioComponents (aumu + aumf, say)
#   [DATA_DIR <dir> [DATA_SUBDIRS <sub>...]] [OBJCXX_SOURCES <f>...])
#
# FACTORY_FUNCTION is what AUSDK_COMPONENT_ENTRY(<factory>, <Class>) defines:
# <Class>Factory. The wrapper's ObjC classes get a per-target name prefix
# (MZGL_AUV2_CLASS_PREFIX) so two of our components in one host don't clash.
function(mzgl_auv2_configure_bundle target)
  cmake_parse_arguments(ARG ""
    "PRODUCT_NAME;BUNDLE_ID;VERSION;AU_TYPE;AU_SUBTYPE;AU_MANUFACTURER;AU_NAME;AU_DESCRIPTION;FACTORY_FUNCTION;COPYRIGHT;INFO_PLIST;DATA_DIR;AU_VERSION_INT"
    "AU_TAGS;DATA_SUBDIRS;OBJCXX_SOURCES;EXTRA_FACTORY_FUNCTIONS" ${ARGN})
  foreach(req PRODUCT_NAME BUNDLE_ID VERSION AU_TYPE AU_SUBTYPE AU_MANUFACTURER AU_NAME FACTORY_FUNCTION)
    if(NOT ARG_${req})
      message(FATAL_ERROR "mzgl_auv2_configure_bundle: ${req} is required")
    endif()
  endforeach()
  if(NOT APPLE OR CMAKE_SYSTEM_NAME MATCHES "iOS")
    message(FATAL_ERROR "mzgl_auv2_configure_bundle: AUv2 components are macOS only")
  endif()
  if(NOT TARGET ausdk)
    message(FATAL_ERROR "mzgl_auv2_configure_bundle: call mzgl_fetch_auv2_sdk() first")
  endif()
  if(NOT ARG_AU_DESCRIPTION)
    set(ARG_AU_DESCRIPTION "${ARG_PRODUCT_NAME}")
  endif()
  if(NOT ARG_COPYRIGHT)
    set(ARG_COPYRIGHT "")
  endif()
  if(NOT ARG_INFO_PLIST)
    set(ARG_INFO_PLIST ${MZGL_PLUGIN_RESOURCE_DIR}/Info-auv2.plist.in)
  endif()

  # Plist variables consumed by Info-auv2.plist.in (configure_file @ONLY).
  set(AUV2_PRODUCT_NAME "${ARG_PRODUCT_NAME}")
  set(AUV2_BUNDLE_ID "${ARG_BUNDLE_ID}")
  set(AUV2_VERSION "${ARG_VERSION}")
  set(AUV2_TYPE "${ARG_AU_TYPE}")
  set(AUV2_SUBTYPE "${ARG_AU_SUBTYPE}")
  set(AUV2_MANUFACTURER "${ARG_AU_MANUFACTURER}")
  set(AUV2_NAME "${ARG_AU_NAME}")
  set(AUV2_DESCRIPTION "${ARG_AU_DESCRIPTION}")
  set(AUV2_FACTORY_FUNCTION "${ARG_FACTORY_FUNCTION}")
  set(AUV2_COPYRIGHT "${ARG_COPYRIGHT}")
  if(ARG_AU_VERSION_INT)
    set(AUV2_VERSION_INT "${ARG_AU_VERSION_INT}")
  else()
    mzgl_au_packed_version(AUV2_VERSION_INT "${ARG_VERSION}")
  endif()
  mzgl_au_tags_xml(AUV2_TAGS_XML ${ARG_AU_TAGS})
  set(_plist ${CMAKE_CURRENT_BINARY_DIR}/${target}-Info.plist)
  configure_file(${ARG_INFO_PLIST} ${_plist} @ONLY)

  # ObjC class names must be identifiers ("05_PluginAUv2" -> "_05_PluginAUv2").
  string(MAKE_C_IDENTIFIER "${target}" _class_prefix)
  target_compile_definitions(${target} PRIVATE MZGL_PLUGIN_AUV2 MZGL_AUV2_CLASS_PREFIX=${_class_prefix}
                                               ${MZGL_BACKEND_COMPILE_DEFS})
  target_include_directories(${target} PRIVATE ${MZGL_PLUGIN_SRC_DIR}/auv2)
  target_link_libraries(${target} PRIVATE ausdk "-framework Cocoa" "-framework AudioToolbox"
                                          "-framework AudioUnit" "-framework CoreAudioKit")

  # Only the component factory function is exported (ObjC classes are found
  # through the runtime, not the symbol table).
  set(_exports ${CMAKE_CURRENT_BINARY_DIR}/${target}-exports.exp)
  set(_export_lines "_${ARG_FACTORY_FUNCTION}\n")
  foreach(fn ${ARG_EXTRA_FACTORY_FUNCTIONS})
    string(APPEND _export_lines "_${fn}\n")
  endforeach()
  file(WRITE ${_exports} "${_export_lines}")
  target_link_options(${target} PRIVATE "LINKER:-exported_symbols_list,${_exports}")
  set_property(TARGET ${target} APPEND PROPERTY LINK_DEPENDS ${_exports})

  set_target_properties(${target} PROPERTIES
    BUNDLE TRUE
    BUNDLE_EXTENSION component
    SUFFIX .component
    OUTPUT_NAME "${ARG_PRODUCT_NAME}"
    XCODE_ATTRIBUTE_WRAPPER_EXTENSION component
    XCODE_ATTRIBUTE_PRODUCT_NAME "${ARG_PRODUCT_NAME}"
    MACOSX_BUNDLE_INFO_PLIST ${_plist})

  _mzgl_plugin_bundle_post_build(${target} "${ARG_DATA_DIR}" "${ARG_DATA_SUBDIRS}")
  install(TARGETS ${target} DESTINATION "$ENV{HOME}/Library/Audio/Plug-Ins/Components")

  set_source_files_properties(${MZGL_PLUGIN_SRC_DIR}/auv2/MzglAUv2View.mm ${ARG_OBJCXX_SOURCES}
                              PROPERTIES COMPILE_FLAGS "-x objective-c++ -fobjc-arc")
endfunction()

# mzgl_add_auv2_plugin(<target>
#   <all mzgl_auv2_configure_bundle args>
#   SOURCES <plugin sources>          must include the MzglAUv2Effect subclass +
#                                     AUSDK_COMPONENT_ENTRY
#   [EXTRA_DEFINITIONS <d>...] [LINK_LIBRARIES <lib>...])
function(mzgl_add_auv2_plugin target)
  cmake_parse_arguments(ARG ""
    "PRODUCT_NAME;BUNDLE_ID;VERSION;AU_TYPE;AU_SUBTYPE;AU_MANUFACTURER;AU_NAME;AU_DESCRIPTION;FACTORY_FUNCTION;COPYRIGHT;INFO_PLIST;DATA_DIR;AU_VERSION_INT"
    "AU_TAGS;DATA_SUBDIRS;OBJCXX_SOURCES;SOURCES;EXTRA_DEFINITIONS;LINK_LIBRARIES;EXTRA_FACTORY_FUNCTIONS" ${ARGN})
  mzgl_auv2_wrapper_sources(_wrapper)
  add_library(${target} MODULE ${ARG_SOURCES} ${_wrapper})
  target_link_libraries(${target} PRIVATE libmzgl ${ARG_LINK_LIBRARIES})
  target_compile_definitions(${target} PRIVATE ${ARG_EXTRA_DEFINITIONS})
  set(_fwd)
  foreach(k PRODUCT_NAME BUNDLE_ID VERSION AU_TYPE AU_SUBTYPE AU_MANUFACTURER AU_NAME AU_DESCRIPTION
            FACTORY_FUNCTION COPYRIGHT INFO_PLIST DATA_DIR AU_VERSION_INT)
    if(ARG_${k})
      list(APPEND _fwd ${k} "${ARG_${k}}")
    endif()
  endforeach()
  foreach(k AU_TAGS DATA_SUBDIRS OBJCXX_SOURCES EXTRA_FACTORY_FUNCTIONS)
    if(ARG_${k})
      list(APPEND _fwd ${k} ${ARG_${k}})
    endif()
  endforeach()
  mzgl_auv2_configure_bundle(${target} ${_fwd})
endfunction()

# ---------------------------------------------------------------------------
# AUv3 (app extension + container app) - Xcode generator only
# ---------------------------------------------------------------------------

# mzgl_add_auv3_plugin(<target>
#   PRODUCT_NAME <name>              container app is "<name>.app", appex "<target>.appex"
#   BUNDLE_ID <app id>               appex gets "<app id>.AUv3" unless AUV3_BUNDLE_ID given
#   VERSION <x.y.z> [BUILD_NUMBER <n>]
#   AU_TYPE <aufx|aumf|aumu> AU_SUBTYPE <4cc> AU_MANUFACTURER <4cc>
#   AU_NAME "<Manufacturer: Product>" [AU_DESCRIPTION <s>] [AU_TAGS <tag>...]
#   SOURCES <plugin sources>         the ::Plugin, PluginEditor, entry points
#   [PRINCIPAL_CLASS <ObjC name>]    default <target>ViewController; generated here
#   [VIEW_WIDTH <pt> VIEW_HEIGHT <pt>] initial editor size (default 500x400)
#   [AUV3_BUNDLE_ID <id>] [APP_TARGET <name>]  default <target>App
#   [DEPLOYMENT_TARGET <ver>]        default 11.0
#   [DEVELOPMENT_TEAM <id>]          automatic signing with "Apple Development";
#                                    without it the products are ad-hoc signed
#                                    (fine for local testing, not distributable)
#   [COPYRIGHT <s>] [ICON <file.icns>]
#   [OBJCXX_SOURCES <f>...] [EXTRA_DEFINITIONS <d>...] [LINK_LIBRARIES <lib>...]
#   [DATA_DIR <dir> [DATA_SUBDIRS <sub>...]])
#
# The AUv3 itself is mzgl's AudioUnitViewController / MZGLEffectAU (in libmzgl);
# they call instantiatePlugin() / instantiatePluginEditor(). What's generated
# here is the principal class (a subclass, so the symbol lands in the appex
# binary), its nib, both Info.plists and the container app, which macOS needs
# because it only discovers AUv3s embedded in an application bundle.
function(mzgl_add_auv3_plugin target)
  cmake_parse_arguments(ARG ""
    "PRODUCT_NAME;BUNDLE_ID;AUV3_BUNDLE_ID;APP_TARGET;VERSION;BUILD_NUMBER;AU_TYPE;AU_SUBTYPE;AU_MANUFACTURER;AU_NAME;AU_DESCRIPTION;PRINCIPAL_CLASS;VIEW_WIDTH;VIEW_HEIGHT;DEPLOYMENT_TARGET;DEVELOPMENT_TEAM;COPYRIGHT;ICON;DATA_DIR"
    "AU_TAGS;SOURCES;OBJCXX_SOURCES;EXTRA_DEFINITIONS;LINK_LIBRARIES;DATA_SUBDIRS" ${ARGN})
  foreach(req PRODUCT_NAME BUNDLE_ID VERSION AU_TYPE AU_SUBTYPE AU_MANUFACTURER AU_NAME SOURCES)
    if(NOT ARG_${req})
      message(FATAL_ERROR "mzgl_add_auv3_plugin: ${req} is required")
    endif()
  endforeach()
  if(NOT APPLE OR CMAKE_SYSTEM_NAME MATCHES "iOS")
    message(STATUS "mzgl_add_auv3_plugin(${target}): macOS only - skipped")
    return()
  endif()
  if(NOT XCODE)
    message(STATUS "mzgl_add_auv3_plugin(${target}): needs the Xcode generator (cmake -G Xcode) - skipped")
    return()
  endif()
  if(NOT ARG_AUV3_BUNDLE_ID)
    set(ARG_AUV3_BUNDLE_ID "${ARG_BUNDLE_ID}.AUv3")
  endif()
  if(NOT ARG_APP_TARGET)
    set(ARG_APP_TARGET "${target}App")
  endif()
  if(NOT ARG_BUILD_NUMBER)
    set(ARG_BUILD_NUMBER 1)
  endif()
  if(NOT ARG_AU_DESCRIPTION)
    set(ARG_AU_DESCRIPTION "${ARG_PRODUCT_NAME}")
  endif()
  if(NOT ARG_PRINCIPAL_CLASS)
    # An ObjC class name must be an identifier ("05_PluginAUv3" is not).
    string(MAKE_C_IDENTIFIER "${target}" _principal_base)
    set(ARG_PRINCIPAL_CLASS "${_principal_base}ViewController")
  endif()
  if(NOT ARG_VIEW_WIDTH)
    set(ARG_VIEW_WIDTH 500)
  endif()
  if(NOT ARG_VIEW_HEIGHT)
    set(ARG_VIEW_HEIGHT 400)
  endif()
  if(NOT ARG_DEPLOYMENT_TARGET)
    set(ARG_DEPLOYMENT_TARGET 11.0)
  endif()
  if(NOT ARG_COPYRIGHT)
    set(ARG_COPYRIGHT "")
  endif()

  # PRODUCT_MODULE_NAME has to be a C identifier ("05_PluginAUv3" is not).
  string(MAKE_C_IDENTIFIER "${target}" _appex_module)
  string(MAKE_C_IDENTIFIER "${ARG_APP_TARGET}" _app_module)

  set(_gen ${CMAKE_CURRENT_BINARY_DIR}/${target}-generated)
  file(MAKE_DIRECTORY ${_gen})

  # Variables for the templates.
  set(AUV3_PRODUCT_NAME "${ARG_PRODUCT_NAME}")
  set(AUV3_PRINCIPAL_CLASS "${ARG_PRINCIPAL_CLASS}")
  set(AUV3_VIEW_WIDTH "${ARG_VIEW_WIDTH}")
  set(AUV3_VIEW_HEIGHT "${ARG_VIEW_HEIGHT}")
  set(AUV3_VERSION "${ARG_VERSION}")
  set(AUV3_BUILD_NUMBER "${ARG_BUILD_NUMBER}")
  set(AUV3_TYPE "${ARG_AU_TYPE}")
  set(AUV3_SUBTYPE "${ARG_AU_SUBTYPE}")
  set(AUV3_MANUFACTURER "${ARG_AU_MANUFACTURER}")
  set(AUV3_NAME "${ARG_AU_NAME}")
  set(AUV3_DESCRIPTION "${ARG_AU_DESCRIPTION}")
  set(AUV3_COPYRIGHT "${ARG_COPYRIGHT}")
  mzgl_au_packed_version(AUV3_VERSION_INT "${ARG_VERSION}")
  mzgl_au_tags_xml(AUV3_TAGS_XML ${ARG_AU_TAGS})
  if(ARG_ICON)
    get_filename_component(AUV3_ICON_FILE "${ARG_ICON}" NAME)
  else()
    set(AUV3_ICON_FILE "")
  endif()

  set(_principal_mm ${_gen}/${ARG_PRINCIPAL_CLASS}.mm)
  set(_xib ${_gen}/${ARG_PRINCIPAL_CLASS}.xib)
  set(_appex_plist ${_gen}/Info-appex.plist)
  set(_app_plist ${_gen}/Info-app.plist)
  configure_file(${MZGL_PLUGIN_SRC_DIR}/auv3/MzglAUv3PrincipalClass.mm.in ${_principal_mm} @ONLY)
  configure_file(${MZGL_PLUGIN_RESOURCE_DIR}/AUViewController.xib.in ${_xib} @ONLY)
  configure_file(${MZGL_PLUGIN_RESOURCE_DIR}/Info-auv3.plist.in ${_appex_plist} @ONLY)
  configure_file(${MZGL_PLUGIN_RESOURCE_DIR}/Info-auv3-app.plist.in ${_app_plist} @ONLY)

  # --- app extension -------------------------------------------------------
  add_executable(${target} MACOSX_BUNDLE ${ARG_SOURCES} ${_principal_mm} ${_xib})
  set_source_files_properties(${_xib} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
  set_source_files_properties(${_principal_mm} ${ARG_OBJCXX_SOURCES}
                              PROPERTIES COMPILE_FLAGS "-x objective-c++ -fobjc-arc")
  target_compile_definitions(${target} PRIVATE MZGL_PLUGIN MZGL_PLUGIN_AUV3 ${MZGL_BACKEND_COMPILE_DEFS}
                                               ${ARG_EXTRA_DEFINITIONS})
  target_link_libraries(${target} PRIVATE libmzgl ${ARG_LINK_LIBRARIES}
                                          "-framework Cocoa" "-framework CoreAudioKit"
                                          "-framework AudioToolbox" "-framework AVFoundation")
  set_target_properties(${target} PROPERTIES
    BUNDLE TRUE
    BUNDLE_EXTENSION appex
    XCODE_PRODUCT_TYPE "com.apple.product-type.app-extension"
    XCODE_ATTRIBUTE_WRAPPER_EXTENSION "appex"
    XCODE_ATTRIBUTE_PRODUCT_NAME "${target}"
    XCODE_ATTRIBUTE_PRODUCT_MODULE_NAME "${_appex_module}"
    XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "${ARG_AUV3_BUNDLE_ID}"
    # Info-auv3.plist.in is a PARTIAL plist (NSExtension/AudioComponents etc.);
    # the identity keys are synthesised by Xcode.
    XCODE_ATTRIBUTE_GENERATE_INFOPLIST_FILE "YES"
    MACOSX_BUNDLE_INFO_PLIST "${_appex_plist}"
    XCODE_ATTRIBUTE_CODE_SIGN_ENTITLEMENTS "${MZGL_PLUGIN_RESOURCE_DIR}/auv3.entitlements"
    XCODE_ATTRIBUTE_SKIP_INSTALL "YES"
    XCODE_ATTRIBUTE_CURRENT_PROJECT_VERSION "${ARG_BUILD_NUMBER}"
    XCODE_ATTRIBUTE_MARKETING_VERSION "${ARG_VERSION}"
    XCODE_ATTRIBUTE_MACOSX_DEPLOYMENT_TARGET "${ARG_DEPLOYMENT_TARGET}"
    XCODE_ATTRIBUTE_LD_RUNPATH_SEARCH_PATHS
      "$(inherited) @executable_path/../Frameworks @executable_path/../../../../Frameworks")
  _mzgl_auv3_signing(${target} "${ARG_DEVELOPMENT_TEAM}")

  if(ARG_DATA_DIR)
    add_custom_command(TARGET ${target} POST_BUILD
      COMMAND "${MZGL_PLUGIN_CMAKE_DIR}/copy-appex-data.sh" "${ARG_DATA_DIR}" ${ARG_DATA_SUBDIRS}
      COMMENT "[${target}] copying data/ into the appex" VERBATIM)
  endif()

  # --- container app -------------------------------------------------------
  set(_app_sources ${MZGL_PLUGIN_SRC_DIR}/auv3/MzglAUv3ContainerApp.mm)
  if(ARG_ICON)
    list(APPEND _app_sources ${ARG_ICON})
    set_source_files_properties(${ARG_ICON} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
  endif()
  add_executable(${ARG_APP_TARGET} MACOSX_BUNDLE ${_app_sources})
  set_source_files_properties(${MZGL_PLUGIN_SRC_DIR}/auv3/MzglAUv3ContainerApp.mm
                              PROPERTIES COMPILE_FLAGS "-x objective-c++ -fobjc-arc")
  target_compile_definitions(${ARG_APP_TARGET} PRIVATE
    "MZGL_AUV3_PRODUCT_NAME=\"${ARG_PRODUCT_NAME}\""
    "MZGL_AUV3_AU_NAME=\"${ARG_AU_NAME}\"")
  target_link_libraries(${ARG_APP_TARGET} PRIVATE "-framework Cocoa")
  set_target_properties(${ARG_APP_TARGET} PROPERTIES
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_INFO_PLIST "${_app_plist}"
    OUTPUT_NAME "${ARG_PRODUCT_NAME}"
    XCODE_ATTRIBUTE_PRODUCT_NAME "${ARG_PRODUCT_NAME}"
    XCODE_ATTRIBUTE_PRODUCT_MODULE_NAME "${_app_module}"
    XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "${ARG_BUNDLE_ID}"
    XCODE_ATTRIBUTE_CODE_SIGN_ENTITLEMENTS "${MZGL_PLUGIN_RESOURCE_DIR}/auv3-app.entitlements"
    XCODE_ATTRIBUTE_SKIP_INSTALL "NO"
    XCODE_ATTRIBUTE_INSTALL_PATH "$(LOCAL_APPS_DIR)"
    XCODE_ATTRIBUTE_CURRENT_PROJECT_VERSION "${ARG_BUILD_NUMBER}"
    XCODE_ATTRIBUTE_MARKETING_VERSION "${ARG_VERSION}"
    XCODE_ATTRIBUTE_MACOSX_DEPLOYMENT_TARGET "${ARG_DEPLOYMENT_TARGET}"
    XCODE_GENERATE_SCHEME ON
    # Embed the appex into Contents/PlugIns, code-signed on copy.
    XCODE_EMBED_APP_EXTENSIONS "${target}"
    XCODE_EMBED_APP_EXTENSIONS_CODE_SIGN_ON_COPY ON)
  _mzgl_auv3_signing(${ARG_APP_TARGET} "${ARG_DEVELOPMENT_TEAM}")
  add_dependencies(${ARG_APP_TARGET} ${target})
endfunction()

function(_mzgl_auv3_signing target team)
  if(team)
    set_target_properties(${target} PROPERTIES
      XCODE_ATTRIBUTE_DEVELOPMENT_TEAM "${team}"
      XCODE_ATTRIBUTE_CODE_SIGN_STYLE "Automatic"
      XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "Apple Development"
      XCODE_ATTRIBUTE_ENABLE_HARDENED_RUNTIME "YES")
  else()
    # "Sign to Run Locally": ad hoc. Enough for pluginkit to register the appex
    # on this machine; re-sign with Developer ID before shipping.
    set_target_properties(${target} PROPERTIES
      XCODE_ATTRIBUTE_CODE_SIGN_STYLE "Manual"
      XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "-"
      XCODE_ATTRIBUTE_DEVELOPMENT_TEAM ""
      XCODE_ATTRIBUTE_PROVISIONING_PROFILE_SPECIFIER "")
  endif()
endfunction()
