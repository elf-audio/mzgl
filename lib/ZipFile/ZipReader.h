#pragma once

#include "filesystem.h"
#include <vector>
#include <memory>

class ZipReaderFile;

class ZipReader {
public:
	ZipReader(const std::string &path);

	std::shared_ptr<ZipReaderFile> open(std::string pathInZip);

	std::vector<std::string> list(bool print = false);

	struct Entry {
		std::string path;
		int offset;
		int size;
		bool valid;
		Entry(const std::string &path, int offset, int size)
			: path(path)
			, offset(offset)
			, size(size)
			, valid(true) {}
		Entry()
			: valid(false) {}
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

	int fileStart;
	int fileSize;

	enum class SeekOrigin {
		Start,
		Current,
	};

	bool seek(int offset, SeekOrigin origin = SeekOrigin::Start);

	size_t read(int8_t *d, size_t sz);
	std::vector<int8_t> read();
	void extract(const std::string &path);

private:
	size_t readSome(std::vector<int8_t> &d);
	std::ifstream zip;
};
