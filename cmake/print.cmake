string(ASCII 27 RESET_COLOUR_CODE)
set(RED "31")
set(GREEN "32")
set(YELLOW "33")
set(BLUE "34")
set(MAGENTA "35")
set(CYAN "36")
set(GREY "90")

function(mzgl_print MESSAGE_TYPE PRINTABLE COLOUR)
  message(${MESSAGE_TYPE}
          "${RESET_COLOUR_CODE}[1;${COLOUR}m${PRINTABLE}${RESET_COLOUR_CODE}[m")
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
