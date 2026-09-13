//
//  ZipIO.h
//
//  Internal helpers shared by ZipFile / ZipReader. Not part of the public API.
//

#pragma once

#include <cstdio>
#include <cstring>
#include <fstream>
#include <string>
#include "filesystem.h"

namespace zipio {

// fopen() that takes an fs::path and handles wide paths on Windows.
inline FILE *openFile(const fs::path &path, const char *mode) {
#ifdef _WIN32
	std::wstring wmode(mode, mode + std::strlen(mode));
	return _wfopen(path.wstring().c_str(), wmode.c_str());
#else
	return std::fopen(path.string().c_str(), mode);
#endif
}

inline std::ifstream openInput(const fs::path &path) {
#ifdef _WIN32
	return std::ifstream(path.wstring(), std::ios::binary);
#else
	return std::ifstream(path, std::ios::binary);
#endif
}

inline std::ofstream openOutput(const fs::path &path) {
#ifdef _WIN32
	return std::ofstream(path.wstring(), std::ios::binary | std::ios::trunc);
#else
	return std::ofstream(path, std::ios::binary | std::ios::trunc);
#endif
}

} // namespace zipio
