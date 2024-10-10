function(mzgl_save_cmake_log_level)
  set(_saved_CMAKE_MESSAGE_LOG_LEVEL
      ${CMAKE_MESSAGE_LOG_LEVEL}
      PARENT_SCOPE)
  set(CMAKE_MESSAGE_LOG_LEVEL
      ERROR
      PARENT_SCOPE)
endfunction()

function(mzgl_restore_cmake_log_level)
  set(CMAKE_MESSAGE_LOG_LEVEL
      ${_saved_CMAKE_MESSAGE_LOG_LEVEL}
      PARENT_SCOPE)
endfunction()
