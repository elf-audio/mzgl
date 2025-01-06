#pragma once
// MZGL_IOS, MZGL_MAC, MZGL_WIN, MZGL_ANDROID, MZGL_LINUX
#ifdef __APPLE__
#	include <TargetConditionals.h>
#	if TARGET_OS_IOS
#		define MZGL_IOS 1
#		if __has_builtin(__builtin_available)
#			define IS_ON_IOS_VERSION_OR_LATER(version) __builtin_available(iOS version, *)
#		else
#			define IS_ON_IOS_VERSION_OR_LATER(version) false
#		endif
#	else
#		define MZGL_MAC 1
#	endif
#elif defined(_WIN32) || defined(_WIN64)
#	define MZGL_WIN 1
#elif defined(__ANDROID__)
#	define MZGL_ANDROID 1
#elif defined(__linux__)
#	define MZGL_LINUX 1
#endif