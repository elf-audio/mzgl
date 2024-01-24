function (mzgl_print PRINTABLE COLOUR)
  string (ASCII 27 Esc)
  message (STATUS "${Esc}[1;${COLOUR}m${PRINTABLE}${Esc}[m")
endfunction ()

function (mzgl_print_verbose PRINTABLE COLOUR)
  string (ASCII 27 Esc)
  message (VERBOSE "${Esc}[1;${COLOUR}m${PRINTABLE}${Esc}[m")
endfunction ()

function (mzgl_print_in_red PRINTABLE)
  mzgl_print ("${PRINTABLE}" 31)
endfunction ()

function (mzgl_print_in_green PRINTABLE)
  mzgl_print ("${PRINTABLE}" 32)
endfunction ()

function (mzgl_print_in_yellow PRINTABLE)
  mzgl_print ("${PRINTABLE}" 33)
endfunction ()

function (mzgl_print_in_blue PRINTABLE)
  mzgl_print ("${PRINTABLE}" 34)
endfunction ()

function (mzgl_print_in_magenta PRINTABLE)
  mzgl_print ("${PRINTABLE}" 35)
endfunction ()

function (mzgl_print_in_cyan PRINTABLE)
  mzgl_print ("${PRINTABLE}" 36)
endfunction ()

function (mzgl_print_verbose_in_red PRINTABLE)
  mzgl_print_verbose ("${PRINTABLE}" 31)
endfunction ()

function (mzgl_print_verbose_in_green PRINTABLE)
  mzgl_print_verbose ("${PRINTABLE}" 32)
endfunction ()

function (mzgl_print_verbose_in_yellow PRINTABLE)
  mzgl_print_verbose ("${PRINTABLE}" 33)
endfunction ()

function (mzgl_print_verbose_in_blue PRINTABLE)
  mzgl_print_verbose ("${PRINTABLE}" 34)
endfunction ()

function (mzgl_print_verbose_in_magenta PRINTABLE)
  mzgl_print_verbose ("${PRINTABLE}" 35)
endfunction ()

function (mzgl_print_verbose_in_cyan PRINTABLE)
  mzgl_print_verbose ("${PRINTABLE}" 36)
endfunction ()
