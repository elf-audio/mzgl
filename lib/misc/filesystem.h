// on windows, use std::filesystem

// #ifdef WIN32
// #include <filesystem>
// namespace fs = std::filesystem;
// #else
// #include <boost/filesystem.hpp>
// namespace fs = boost::filesystem;
// #endif
#include <ghc/filesystem.hpp>
namespace fs = ghc::filesystem;