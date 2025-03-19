string(ASCII 27 RESET_COLOUR_CODE)
set(RED "31")
set(GREEN "32")
set(YELLOW "33")
set(BLUE "34")
set(MAGENTA "35")
set(CYAN "36")
set(GREY "90")

set(CAN_DO_COLOR FALSE)
if(DEFINED ENV{CLION_IDE} AND "$ENV{CLION_IDE}" STREQUAL "TRUE")
  set(CAN_DO_COLOR TRUE)
elseif(DEFINED ENV{TERM_PROGRAM} AND "$ENV{TERM_PROGRAM}" STREQUAL
                                     "Apple_Terminal")
  set(CAN_DO_COLOR TRUE)
elseif(DEFINED ENV{TERM_PROGRAM} AND "$ENV{TERM_PROGRAM}" STREQUAL "iTerm.app")
  set(CAN_DO_COLOR TRUE)
elseif(DEFINED ENV{TERM} AND "$ENV{TERM}" MATCHES
                             "xterm.*|screen.*|color.*|ansi.*")
  set(CAN_DO_COLOR TRUE)
endif()

if(NOT CAN_DO_COLOR)
  message(DEBUG "No color support detected")
endif()

function(mzgl_print MESSAGE_TYPE PRINTABLE COLOUR)
  if(NOT CAN_DO_COLOR)
    message(${MESSAGE_TYPE} "${PRINTABLE}")
  else()
    message(
      ${MESSAGE_TYPE}
      "${RESET_COLOUR_CODE}[1;${COLOUR}m${PRINTABLE}${RESET_COLOUR_CODE}[m")
  endif()
endfunction()

function(mzgl_print_in_red PRINTABLE)
  mzgl_print(STATUS "${PRINTABLE}" "${RED}")
endfunction()

function(mzgl_print_in_green PRINTABLE)
  mzgl_print(STATUS "${PRINTABLE}" "${GREEN}")
endfunction()

function(mzgl_print_in_yellow PRINTABLE)
  mzgl_print(STATUS "${PRINTABLE}" "${YELLOW}")
endfunction()

function(mzgl_print_in_blue PRINTABLE)
  mzgl_print(STATUS "${PRINTABLE}" "${BLUE}")
endfunction()

function(mzgl_print_in_magenta PRINTABLE)
  mzgl_print(STATUS "${PRINTABLE}" "${MAGENTA}")
endfunction()

function(mzgl_print_in_cyan PRINTABLE)
  mzgl_print(STATUS "${PRINTABLE}" "${CYAN}")
endfunction()

function(mzgl_print_in_grey PRINTABLE)
  mzgl_print(STATUS "${PRINTABLE}" "${GREY}")
endfunction()

function(mzgl_print_verbose_in_red PRINTABLE)
  mzgl_print(VERBOSE "${PRINTABLE}" "${RED}")
endfunction()

function(mzgl_print_verbose_in_green PRINTABLE)
  mzgl_print(VERBOSE "${PRINTABLE}" "${GREEN}")
endfunction()

function(mzgl_print_verbose_in_yellow PRINTABLE)
  mzgl_print(VERBOSE "${PRINTABLE}" "${YELLOW}")
endfunction()

function(mzgl_print_verbose_in_blue PRINTABLE)
  mzgl_print(VERBOSE "${PRINTABLE}" "${BLUE}")
endfunction()

function(mzgl_print_verbose_in_magenta PRINTABLE)
  mzgl_print(VERBOSE "${PRINTABLE}" "${MAGENTA}")
endfunction()

function(mzgl_print_verbose_in_cyan PRINTABLE)
  mzgl_print(VERBOSE "${PRINTABLE}" "${CYAN}")
endfunction()

function(mzgl_print_verbose_in_grey PRINTABLE)
  mzgl_print(VERBOSE "${PRINTABLE}" "${GREY}")
endfunction()

function(mzgl_print_debug_in_red PRINTABLE)
  mzgl_print(DEBUG "${PRINTABLE}" "${RED}")
endfunction()

function(mzgl_print_debug_in_green PRINTABLE)
  mzgl_print(DEBUG "${PRINTABLE}" "${GREEN}")
endfunction()

function(mzgl_print_debug_in_yellow PRINTABLE)
  mzgl_print(DEBUG "${PRINTABLE}" "${YELLOW}")
endfunction()

function(mzgl_print_debug_in_blue PRINTABLE)
  mzgl_print(DEBUG "${PRINTABLE}" "${BLUE}")
endfunction()

function(mzgl_print_debug_in_magenta PRINTABLE)
  mzgl_print(DEBUG "${PRINTABLE}" "${MAGENTA}")
endfunction()

function(mzgl_print_debug_in_cyan PRINTABLE)
  mzgl_print(DEBUG "${PRINTABLE}" "${CYAN}")
endfunction()

function(mzgl_print_debug_in_grey PRINTABLE)
  mzgl_print(DEBUG "${PRINTABLE}" "${GREY}")
endfunction()
