
#ifdef __APPLE__
#	include <TargetConditionals.h>
#	if TARGET_OS_IOS
#		define __IOS
#	else
#		define __MAC
#	endif
#elif !defined(__ANDROID__) && !defined(__arm__)
#	define __WIN
#elif defined(__ANDROID__)
#	define __ANDROID
#else
#	define __RPI
#endif
