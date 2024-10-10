# Wrap your cmake call in this, to disable the warnings
#
# If your call to add package or similar generates a lot of warnings about the
# cmake version (or simialr) then you can wrap it in this call and it will be
# suprressed. An example would be
# mzgl_call_with_warnings_disabled(mzgl_add_package("gh:jatinchowdhury18/RTNeural#e85e7c4"))
#
# @param CALLABLE The function to call
function(mzgl_call_with_warnings_disabled CALLABLE)
  cmake_policy(PUSH)
  cmake_policy(SET CMP0077 NEW)
  cmake_policy(SET CMP0091 NEW)
  set(CMAKE_WARN_DEPRECATED
      OFF
      CACHE INTERNAL "No deprecated warnings")
  set(_saved_CMAKE_SUPPRESS_DEVELOPER_WARNINGS
      ${CMAKE_SUPPRESS_DEVELOPER_WARNINGS})
  set(CMAKE_SUPPRESS_DEVELOPER_WARNINGS
      1
      CACHE INTERNAL "Suppress developer warnings" FORCE)

  execute_process(COMMAND ${CALLABLE})

  set(CMAKE_SUPPRESS_DEVELOPER_WARNINGS
      ${_saved_CMAKE_SUPPRESS_DEVELOPER_WARNINGS}
      CACHE INTERNAL "Restore original developer warnings" FORCE)
  cmake_policy(POP)
  set(CMAKE_WARN_DEPRECATED
      ON
      CACHE INTERNAL "Restore deprecated warnings")
endfunction()
