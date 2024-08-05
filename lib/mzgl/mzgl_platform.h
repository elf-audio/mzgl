#pragma once
#ifdef __APPLE__
#	include <TargetConditionals.h>
#	if TARGET_OS_IOS
#		define MZGL_IOS
#	else
#		define MZGL_MAC
#	endif
#elif defined(_WIN32)
#elif !defined(__ANDROID__) && !defined(__arm__)
#	define MZGL_WIN
#elif defined(__ANDROID__)
#	define MZGL_ANDROID
#else
#	define MZGL_RPI
#endif
