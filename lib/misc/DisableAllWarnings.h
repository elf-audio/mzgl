#pragma once

// clang-format off
#if __clang__
#define DISABLE_WARNINGS \
        _Pragma("clang diagnostic push") \
        _Pragma("clang diagnostic ignored \"-Weverything\"")
#define RESTORE_WARNINGS \
        _Pragma("clang diagnostic pop")
#elif __GNUC__
#define DISABLE_WARNINGS \
        _Pragma("GCC diagnostic push") \
        _Pragma("GCC diagnostic ignored \"-Wall\"") \
        _Pragma("GCC diagnostic ignored \"-Wpragmas\"") \
        _Pragma("GCC diagnostic ignored \"-Wextra\"") \
        _Pragma("GCC diagnostic ignored \"-Wshadow\"") \
        _Pragma("GCC diagnostic ignored \"-Wunused-parameter\"") \
        _Pragma("GCC diagnostic ignored \"-Wconversion\"") \
        _Pragma("GCC diagnostic ignored \"-Wsign-conversion\"") \
        _Pragma("GCC diagnostic ignored \"-Wsign-compare\"") \
        _Pragma("GCC diagnostic ignored \"-Wfloat-conversion\"") \
        _Pragma("GCC diagnostic ignored \"-Wswitch-enum\"") \
        _Pragma("GCC diagnostic ignored \"-Wswitch\"") \
        _Pragma("GCC diagnostic ignored \"-Wzero-as-null-pointer-constant\"") \
        _Pragma("GCC diagnostic ignored \"-Wunused-variable\"") \
        _Pragma("GCC diagnostic ignored \"-Wredundant-decls\"") \
        _Pragma("GCC diagnostic ignored \"-Wsubobject-linkage\"") \
        _Pragma("GCC diagnostic ignored \"-Wunused-but-set-variable\"") \
        _Pragma("GCC diagnostic ignored \"-Wredundant-move\"") \
        _Pragma("GCC diagnostic ignored \"-Wstrict-aliasing\"") \
        _Pragma("GCC diagnostic ignored \"-Woverloaded-virtual\"") \
        _Pragma("GCC diagnostic ignored \"-Wc99-extensions\"") \
        _Pragma("GCC diagnostic ignored \"-Wmisleading-indentation\"") \
        _Pragma("GCC diagnostic ignored \"-Wmaybe-uninitialized\"") \
        _Pragma("GCC diagnostic ignored \"-Wimplicit-fallthrough\"") \
        _Pragma("GCC diagnostic ignored \"-Wcast-function-type\"") \
        _Pragma("GCC diagnostic ignored \"-Wunused-label\"") \
        _Pragma("GCC diagnostic ignored \"-Wnarrowing\"") \
        _Pragma("GCC diagnostic ignored \"-Wparentheses\"") \
        _Pragma("GCC diagnostic ignored \"-Wwrite-strings\"") \
        _Pragma("GCC diagnostic ignored \"-Wformat-overflow\"") \
        _Pragma("GCC diagnostic ignored \"-Wdeprecated-copy-with-dtor\"") \
        _Pragma("GCC diagnostic ignored \"-Wdeprecated\"") \
        _Pragma("GCC diagnostic ignored \"-Wdeprecated-declarations\"") \
        _Pragma("GCC diagnostic ignored \"-Wuse-after-free\"") \
        _Pragma("GCC diagnostic ignored \"-Warray-bounds\"") \
        _Pragma("GCC diagnostic ignored \"-Wvolatile\"") \
        _Pragma("GCC diagnostic ignored \"-Wmissing-field-initializers\"") \
        _Pragma("GCC diagnostic ignored \"-Wfloat-equal\"") \
        _Pragma("GCC diagnostic ignored \"-Wpedantic\"") \
        _Pragma("GCC diagnostic ignored \"-Wredundant-move\"")
#define RESTORE_WARNINGS \
        _Pragma("GCC diagnostic pop")
#elif _MSC_VER
#define DISABLE_WARNINGS \
        __pragma(warning(push, 0)) \
        __pragma(warning(disable: 2440 2664 4244 4701 4702 4706 4722 6011 6246 6255 6262 6297 6308 6323 6340 6385 6386 28182))
#define RESTORE_WARNINGS \
        __pragma(warning(pop))
#else
#define DISABLE_WARNINGS
#define RESTORE_WARNINGS
#endif
// clang-format on