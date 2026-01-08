#pragma once
// MZGL_IOS, MZGL_MAC, MZGL_WIN, MZGL_ANDROID, MZGL_LINUX

#define MZGL_IOS	 0
#define MZGL_MAC	 0
#define MZGL_WIN	 0
#define MZGL_ANDROID 0
#define MZGL_LINUX	 0

#ifdef __APPLE__
#	include <TargetConditionals.h>
#	if TARGET_OS_IOS
#		undef MZGL_IOS
#		define MZGL_IOS 1
#	else
#		undef MZGL_MAC
#		define MZGL_MAC 1
#	endif
#elif defined(_WIN32) || defined(_WIN64)
#	undef MZGL_WIN
#	define MZGL_WIN 1
#elif defined(__ANDROID__)
#	undef MZGL_ANDROID
#	define MZGL_ANDROID 1
#elif defined(__linux__)
#	undef MZGL_LINUX
#	define MZGL_LINUX 1
#endif

#if __has_builtin(__builtin_available)
#	define IS_ON_IOS_VERSION_OR_LATER(version) __builtin_available(iOS version, *)
#else
#	define IS_ON_IOS_VERSION_OR_LATER(version) false
#endif