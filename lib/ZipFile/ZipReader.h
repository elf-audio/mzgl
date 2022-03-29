#pragma once

#include <fstream>
#include <vector>

class ZipReader {
public:
	std::ifstream zip;

	ZipReader(const std::string &path);

	enum class SeekOrigin {
		Start,
		Current,
	};
 
	bool open(std::string pathInZip);
	bool seek(int offset, SeekOrigin origin = SeekOrigin::Start);

    size_t read(std::vector<int8_t> &d);
    size_t read(int8_t *d, uint32_t sz);
	

	bool printTextFile(const std::string &path);

	std::vector<std::string> list(bool print = false);

	struct Entry {
		std::string path;
		int offset;
		int size;
		bool valid;
		Entry(const std::string &path, int offset, int size) : path(path), offset(offset), size(size), valid(true) {}
		Entry() : valid(false) {}
	};

private:

	void printTextFile(const Entry &e);
	void seekToEntry(const Entry &e);
	
	int currEntryStart;
	int currEntryLength;

	std::vector<Entry> entries;

	Entry findEntry(const std::string &path);
};
