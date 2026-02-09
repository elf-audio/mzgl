#include "ZipReader.h"
#include <string.h>

struct ZipEndOfCD {
	int32_t signature; // 0x50, 0x4b, 0x05, 0x06
	int16_t diskNo;
	int16_t diskCDStart;
	int16_t numCDs;
	int16_t numEntries;
	int32_t cdSizeBytes;
	int32_t cdFileOffset;
	int16_t commentLength;

	void print() {
		printf(
			"disk no: %d\ndisk cd start: %d\nnum CDs: %d\nnum entries: %d\ncd size in bytes: %d\ncd file offset: %d\ncomment length: %d\n",
			diskNo,
			diskCDStart,
			numCDs,
			numEntries,
			cdSizeBytes,
			cdFileOffset,
			commentLength);
	}

	void readFromStream(std::istream &stream) {
		// Helper lambda to read data from the stream
		auto readField = [&stream](auto *destination, size_t size) {
			stream.read(reinterpret_cast<char *>(destination), size);
		};

		readField(&signature, sizeof(signature));
		readField(&diskNo, sizeof(diskNo));
		readField(&diskCDStart, sizeof(diskCDStart));
		readField(&numCDs, sizeof(numCDs));
		readField(&numEntries, sizeof(numEntries));
		readField(&cdSizeBytes, sizeof(cdSizeBytes));
		readField(&cdFileOffset, sizeof(cdFileOffset));
		readField(&commentLength, sizeof(commentLength));

		// At this point, if you wanted to read the comment itself (given its length), you could do so, though it's not part of the struct you provided.
	}
};

struct ZipCDEntry {
	int8_t signature[4]; // 0x50, 0x4b, 0x05, 0x06
	int16_t version;
	int16_t versionNeeded;
	int16_t flags;
	int16_t compression;
	int16_t modTime;
	int16_t modDate;
	int32_t crc32;
	int32_t compressedSize;
	int32_t uncompressedSize;
	int16_t fileNameLength;
	int16_t extraFieldLength;
	int16_t fileCommentLength;
	int16_t diskNoStart;
	int16_t internalAttrs;
	int32_t externalAttrs;
	int32_t localHeaderOffset;
	std::string filename;

	void readFromVector(const std::vector<int8_t> &buffer, size_t offset) {
		// Helper lambda to read data from the buffer
		auto readField = [&buffer, &offset](auto *destination, size_t size) {
			std::memcpy(destination, buffer.data() + offset, size);
			offset += size;
		};

		readField(signature, sizeof(signature));
		readField(&version, sizeof(version));
		readField(&versionNeeded, sizeof(versionNeeded));
		readField(&flags, sizeof(flags));
		readField(&compression, sizeof(compression));
		readField(&modTime, sizeof(modTime));
		readField(&modDate, sizeof(modDate));
		readField(&crc32, sizeof(crc32));
		readField(&compressedSize, sizeof(compressedSize));
		readField(&uncompressedSize, sizeof(uncompressedSize));
		readField(&fileNameLength, sizeof(fileNameLength));
		readField(&extraFieldLength, sizeof(extraFieldLength));
		readField(&fileCommentLength, sizeof(fileCommentLength));
		readField(&diskNoStart, sizeof(diskNoStart));
		readField(&internalAttrs, sizeof(internalAttrs));
		readField(&externalAttrs, sizeof(externalAttrs));
		readField(&localHeaderOffset, sizeof(localHeaderOffset));

		// Read the filename
		if (fileNameLength > 0) {
			filename.assign(reinterpret_cast<const char *>(buffer.data() + offset), fileNameLength);
			offset += fileNameLength;
		} else {
			filename.clear();
		}

		// If you plan to read extraField and comment, add similar code here.
	}

	// ... other member functions ...

	void print() {
		printf(
			"\n--------\nENTRY\nsignature: 0x%x 0x%x 0x%x 0x%x\nversion: %d\nversionNeeded: %d\nflags: %d\ncompression: %d\nmodTime: %d\nmodDate: %d\ncrc32: %d\ncompressedSize: %d\nuncompressedSize: %d\nfileNameLength: %d\nextraFieldLength: %d\nfileCommentLength: %d\ndiskNoStart: %d\ninternalAttrs: %d\nexternalAttrs: %d\nlocalHeaderOffset: %d\n",
			signature[0],
			signature[1],
			signature[2],
			signature[3],
			version,
			versionNeeded,
			flags,
			compression,
			modTime,
			modDate,
			crc32,
			compressedSize,
			uncompressedSize,
			fileNameLength,
			extraFieldLength,
			fileCommentLength,
			diskNoStart,
			internalAttrs,
			externalAttrs,
			localHeaderOffset);
	}

	int getTotalLength() { return 46 + fileNameLength + extraFieldLength + fileCommentLength; }
};

ZipReaderFile::ZipReaderFile(const std::string &zipPath, const ZipReader::Entry &entry) {
	zip.open(zipPath, std::ios::binary);
	if (!zip.good()) {
		printf("Can't read file");
		throw std::runtime_error("Can't read zip file '" + zipPath + "'");
	}

	zip.seekg(entry.offset + 26);
	int16_t fnExtraLength[2];
	zip.read((char *) fnExtraLength, 4);

	fileStart = entry.offset + 30 + fnExtraLength[0] + fnExtraLength[1];
	fileSize  = entry.size;
	zip.seekg(fileStart);
}

bool ZipReaderFile::seek(int offset, SeekOrigin origin) {
	if (origin == SeekOrigin::Start) {
		if (offset < 0 || offset >= fileSize) return false;
		zip.seekg(fileStart + offset);
	} else if (origin == SeekOrigin::Current) {
		int absolute = (int) zip.tellg() - fileStart;
		if (absolute < 0 || absolute >= fileSize) return false;
		zip.seekg(offset, std::ios_base::cur);
	} else {
		throw std::runtime_error("Unknown seek origin in ZipReader");
	}

	return true;
}

size_t ZipReaderFile::read(int8_t *d, size_t sz) {
	int pos = (int) zip.tellg() - fileStart;
	if (fileSize - pos >= sz) {
		zip.read((char *) d, sz);
	} else if (pos >= fileSize) {
		return 0;
	} else {
		zip.read((char *) d, fileSize - pos);
	}
	return zip.gcount();
}

std::vector<int8_t> ZipReaderFile::read() {
	std::vector<int8_t> data;
	std::vector<int8_t> buff(4096);
	while (1) {
		auto amountRead = readSome(buff);
		data.insert(data.end(), buff.begin(), buff.begin() + amountRead);
		if (amountRead != buff.size()) break;
	}
	return data;
}

size_t ZipReaderFile::readSome(std::vector<int8_t> &d) {
	return read(d.data(), d.size());
}

void ZipReaderFile::extract(const std::string &path) {
	seek(0);
	fs::ofstream f(fs::u8path(path), std::ios_base::binary);
	std::vector<int8_t> buff(4096);
	while (1) {
		auto amountRead = readSome(buff);
		f.write((char *) buff.data(), amountRead);
		if (amountRead != buff.size()) break;
	}
}

ZipEndOfCD readEndOfCD(std::ifstream &zip) {
	ZipEndOfCD endOfCd;
	zip.seekg(0, std::ios_base::end);
	int size = static_cast<int>(zip.tellg());

	auto pos = size - 4; // or so?
	while (pos > 0) {
		zip.seekg(pos);
		if (zip.peek() == 0x50) {
			// printf("found 0x50\n");
			char sig[4];
			zip.read(sig, 4);
			if (sig[1] == 0x4b && sig[2] == 0x05 && sig[3] == 0x06) {
				// printf("Found header\n");
				zip.seekg(pos);
				endOfCd.readFromStream(zip);
				//				zip.read((char*)&endOfCd, sizeof(ZipEndOfCD));
				zip.clear(); // clear eof
				break;
			}
		}
		pos--;
	}
	if (pos == 0) {
		printf("Couldn't find header\n");
		throw std::runtime_error("Couldn't find end of cd header");
	}
	return endOfCd;
}

/////////////////////
///
std::vector<ZipReader::Entry> readCD(std::ifstream &zip, const ZipEndOfCD &endOfCd) {
	std::vector<ZipReader::Entry> entries;
	std::vector<int8_t> cd(endOfCd.cdSizeBytes);
	if (!zip.good()) {
		printf("Zip stream broken\n");
		throw std::runtime_error("Zip stream not readable");
	}
	zip.seekg(endOfCd.cdFileOffset, std::ios_base::beg);
	zip.read((char *) cd.data(), cd.size());
	int charsRead = static_cast<int>(zip.gcount());
	if (charsRead != cd.size()) {
		printf("Couldn't read first entry - only read %d chars(%ld)\n", charsRead, cd.size());
		throw std::runtime_error("Couldn't read first CD entry");
	}

	int offset = 0;
	while (offset < cd.size()) {
		ZipCDEntry ent;
		ent.readFromVector(cd, offset);
		//		signed char *fileNamePtr = cd.data() + offset + 46;

		//		int fnLength = ent->fileNameLength;
		//#ifdef _WIN32
		//		char fn[260] = {}; // MAX_PATH
		//#else
		//		char fn[fnLength+1];
		//#endif
		//
		//		fn[fnLength] = 0;
		//		memcpy(fn, fileNamePtr, fnLength);
		//		printf("Compression used: %d\n", ent->compression);
		entries.emplace_back(ent.filename, ent.localHeaderOffset, ent.uncompressedSize);

		offset += ent.getTotalLength();
	}

	return entries;
}
////////////////////////////

ZipReader::ZipReader(const std::string &path) {
	this->zipPath = path;
	std::ifstream zip;
	zip.open(zipPath, std::ios::binary);
	if (!zip.good()) {
		printf("Can't read file");
		throw std::runtime_error("Can't read zip file '" + path + "'");
	}
	auto endOfCd = readEndOfCD(zip);
	entries		 = readCD(zip, endOfCd);
}

std::shared_ptr<ZipReaderFile> ZipReader::open(std::string pathInZip) {
	auto currEntry = findEntry(pathInZip);
	if (!currEntry.valid) return nullptr;
	//	seekToEntry(currEntry);
	return std::make_shared<ZipReaderFile>(zipPath, currEntry);
}

std::vector<std::string> ZipReader::list(bool print) {
	std::vector<std::string> ret;
	for (auto &s: entries) {
		ret.emplace_back(s.path);
		if (print) {
			printf("%s: %d/%d\n", s.path.c_str(), s.offset, s.size);
		}
	}
	return ret;
}

ZipReader::Entry ZipReader::findEntry(const std::string &path) {
	for (const auto &e: entries) {
		if (e.path == path) {
			return e;
		}
	}
	return Entry();
}
