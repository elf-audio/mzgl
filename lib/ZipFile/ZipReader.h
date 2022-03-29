#pragma once

#include <fstream>
#include <vector>


class ZipReaderFile;

class ZipReader {
public:

	ZipReader(const std::string &path);
 
	std::shared_ptr<ZipReaderFile> open(std::string pathInZip);
	
//	bool printTextFile(const std::string &path);

	std::vector<std::string> list(bool print = false);
	
	struct Entry {
		std::string path;
		int offset;
		int size;
		bool valid;
		Entry(const std::string &path, int offset, int size) : path(path), offset(offset), size(size), valid(true) {}
		Entry() : valid(false) {}
	};
	std::vector<Entry> listEntries() const { return entries; }
private:
    std::string zipPath;
//	void printTextFile(const Entry &e);
//	void seekToEntry(const Entry &e);
	
//	int currEntryStart;
//	int currEntryLength;

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
    size_t read(std::vector<int8_t> &d);
    size_t read(int8_t *d, uint32_t sz);
private:
    std::ifstream zip;
};
