#pragma once

#if __clang__
#	pragma clang diagnostic push
#	pragma clang diagnostic ignored "-Weverything"
#elif _MSC_VER
#	pragma warning(push, 0)
#	pragma warning(disable : 4702)
#	pragma warning(disable : 4706)
#endif

#include <ghc/filesystem.hpp>
namespace fs = ghc::filesystem;

#if __clang__
#	pragma clang diagnostic pop
#elif _MSC_VER
#	pragma warning(pop)
#endif