#ifndef _WIN32IO_UTF8_H
#define _WIN32IO_UTF8_H


#if defined(__cplusplus)
extern "C"
{
#endif

	// filename is utf-8 encoded
	void* win32io_utf8_fopen(const char *filename, const char* mode);

#if defined(__cplusplus)
}
#endif


#endif // _WIN32IO_UTF8_H