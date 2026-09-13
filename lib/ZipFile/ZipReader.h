#pragma once

#include "filesystem.h"
#include <cstdint>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

class ZipReaderFile;

/**
 * Random access into the entries of a zip file. Reads the central directory
 * once (via miniz, so zip64 archives work) and then hands out ZipReaderFile
 * objects that read straight from the zip on disk. Stored (uncompressed)
 * entries are seekable and are what sample packs use for streaming playback;
 * deflated entries are inflated into memory when opened.
 *
 * Throws std::runtime_error if the zip can't be opened or parsed.
 */
class ZipReader {
public:
	ZipReader(const std::string &path);

	std::shared_ptr<ZipReaderFile> open(const std::string &pathInZip);

	std::vector<std::string> list(bool print = false);

	struct Entry {
		std::string path;
		uint64_t offset			= 0; // offset of the local file header in the zip
		uint64_t size			= 0; // uncompressed size
		uint64_t compressedSize = 0;
		int compression			= 0; // 0 = STORE, 8 = DEFLATE
		uint32_t index			= 0; // index of the entry in the central directory
		bool valid				= false;

		Entry() = default;
		Entry(const std::string &path,
			  uint64_t offset,
			  uint64_t size,
			  uint64_t compressedSize,
			  int compression,
			  uint32_t index)
			: path(path)
			, offset(offset)
			, size(size)
			, compressedSize(compressedSize)
			, compression(compression)
			, index(index)
			, valid(true) {}
	};
	std::vector<Entry> listEntries() const { return entries; }
	std::string zipPath;

private:
	std::vector<Entry> entries;

	Entry findEntry(const std::string &path);
};

class ZipReaderFile {
public:
	ZipReaderFile(const std::string &zipPath, const ZipReader::Entry &entry);

	uint64_t fileStart = 0; // offset of the entry's data in the zip (stored entries)
	uint64_t fileSize  = 0; // uncompressed size

	enum class SeekOrigin {
		Start,
		Current,
	};

	// Seeks within the entry. Returns false (and doesn't move) if the target is
	// outside [0, fileSize].
	bool seek(int64_t offset, SeekOrigin origin = SeekOrigin::Start);

	size_t read(int8_t *d, size_t sz);
	std::vector<int8_t> read();
	void extract(const std::string &path);

private:
	size_t readSome(std::vector<int8_t> &d);
	uint64_t tell();

	std::ifstream zip;

	// For DEFLATE-compressed entries, we decompress into memory
	std::vector<int8_t> decompressedData;
	uint64_t memPos = 0;
	bool isDeflate	= false;
};
