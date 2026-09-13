//
//  Gzip.h
//
//  Minimal gzip (RFC 1952) writer on top of miniz - a 10 byte header, raw
//  deflate body and CRC32 + size trailer. Replaces gzip-hpp so nothing needs
//  zlib. Used for Ableton .adg export (which is a gzipped XML file).
//

#pragma once

#include <cstddef>
#include <string>

namespace gzip {

// Compression level 0 (store) .. 10 (slowest), 6 is a good default.
constexpr int defaultLevel = 6;

// Throws std::runtime_error if compression fails or size exceeds 4 GB.
std::string compress(const char *data, std::size_t size, int level = defaultLevel);

inline std::string compress(const std::string &data, int level = defaultLevel) {
	return compress(data.data(), data.size(), level);
}

} // namespace gzip
