#include "Gzip.h"
#include "miniz.h"

#include <cstdint>
#include <cstdlib>
#include <limits>
#include <stdexcept>

namespace gzip {

std::string compress(const char *data, std::size_t size, int level) {
	if (size > std::numeric_limits<uint32_t>::max()) {
		throw std::runtime_error("gzip::compress: input larger than 4 GB is not supported");
	}
	if (level < 0) level = 0;
	if (level > 10) level = 10;

	// Raw deflate (negative window bits = no zlib header/trailer).
	int flags = tdefl_create_comp_flags_from_zip_params(level, -MZ_DEFAULT_WINDOW_BITS, MZ_DEFAULT_STRATEGY);
	size_t deflatedSize = 0;
	void *deflated		= tdefl_compress_mem_to_heap(data, size, &deflatedSize, flags);
	if (deflated == nullptr) {
		throw std::runtime_error("gzip::compress: deflate failed");
	}

	std::string out;
	out.reserve(10 + deflatedSize + 8);

	// Header: ID1 ID2 CM FLG MTIME(4) XFL OS
	const unsigned char xfl = level >= 9 ? 2 : (level == 1 ? 4 : 0);
	const unsigned char header[10] = {0x1f, 0x8b, 8, 0, 0, 0, 0, 0, xfl, 3 /* unix */};
	out.append(reinterpret_cast<const char *>(header), sizeof(header));

	out.append(static_cast<const char *>(deflated), deflatedSize);
	std::free(deflated);

	// Trailer: CRC32 and ISIZE, both little endian.
	auto appendLE32 = [&out](uint32_t v) {
		const unsigned char b[4] = {static_cast<unsigned char>(v & 0xff),
									static_cast<unsigned char>((v >> 8) & 0xff),
									static_cast<unsigned char>((v >> 16) & 0xff),
									static_cast<unsigned char>((v >> 24) & 0xff)};
		out.append(reinterpret_cast<const char *>(b), sizeof(b));
	};
	appendLE32(static_cast<uint32_t>(mz_crc32(MZ_CRC32_INIT, reinterpret_cast<const mz_uint8 *>(data), size)));
	appendLE32(static_cast<uint32_t>(size));
	return out;
}

} // namespace gzip
