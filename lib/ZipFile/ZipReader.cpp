#include "ZipReader.h"

#pragma pack(2)
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
		printf("disk no: %d\ndisk cd start: %d\nnum CDs: %d\nnum entries: %d\ncd size in bytes: %d\ncd file offset: %d\ncomment length: %d\n",diskNo,diskCDStart,numCDs,numEntries,cdSizeBytes,cdFileOffset,commentLength);
	}
};
#pragma pack()

#pragma pack(2)
struct ZipLocalFileHeader {
	int32_t signature; // 0x50, 0x4b, 0x03, 0x04
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

	void print() {
		printf("HEADER: \nversion: %d\nversion needed: %d\nflags: %d\ncompression: %d\nmodTime: %d\nmodDate: %d\ncrc32: %d\ncompressedSize: %d\nuncompressedSize: %d\nfileNameLength: %d\nextraFieldLength: %d\n",version,versionNeeded,flags,compression,modTime,modDate,crc32,compressedSize,uncompressedSize,fileNameLength,extraFieldLength);
	}
};
#pragma pack()

#pragma pack(2)
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

	void print() {
		printf("\n--------\nENTRY\nsignature: 0x%x 0x%x 0x%x 0x%x\nversion: %d\nversionNeeded: %d\nflags: %d\ncompression: %d\nmodTime: %d\nmodDate: %d\ncrc32: %d\ncompressedSize: %d\nuncompressedSize: %d\nfileNameLength: %d\nextraFieldLength: %d\nfileCommentLength: %d\ndiskNoStart: %d\ninternalAttrs: %d\nexternalAttrs: %d\nlocalHeaderOffset: %d\n",signature[0],signature[1],signature[2],signature[3], version, versionNeeded, flags, compression, modTime, modDate, crc32, compressedSize, uncompressedSize, fileNameLength, extraFieldLength, fileCommentLength, diskNoStart, internalAttrs, externalAttrs, localHeaderOffset);
	}

	int getTotalLength() {
		return 46 + fileNameLength + extraFieldLength + fileCommentLength;
	}
};
#pragma pack()



ZipReaderFile::ZipReaderFile(const std::string &zipPath, const ZipReader::Entry &entry) {
    zip.open(zipPath, std::ios::binary);
    if(!zip.good()) {
        printf("Can't read file");
        throw std::runtime_error("Can't read zip file '"+zipPath+"'");
    }
    
    zip.seekg(entry.offset + 26);
    int16_t fnExtraLength[2];
    zip.read((char*)fnExtraLength, 4);

    fileStart = entry.offset + 30 + fnExtraLength[0] + fnExtraLength[1];
    fileSize = entry.size;
    zip.seekg(fileStart);
}

bool ZipReaderFile::seek(int offset, SeekOrigin origin) {
    if(origin==SeekOrigin::Start) {
        if(offset<0 || offset>=fileSize) return false;
        zip.seekg(fileStart + offset);
    } else if(origin==SeekOrigin::Current) {
        int absolute = (int)zip.tellg() - fileStart;
        if(absolute<0 || absolute>=fileSize) return false;
        zip.seekg(offset, std::ios_base::cur);
    } else {
        throw std::runtime_error("Unknown seek origin in ZipReader");
    }

    return true;
}


size_t ZipReaderFile::read(int8_t *d, uint32_t sz) {
    int pos = (int)zip.tellg() - fileStart;
    if(fileSize - pos >= sz) {
        zip.read((char*)d, sz);
    } else if(pos>=fileSize) {
        return 0;
    } else {
        zip.read((char*)d, fileSize - pos);
    }
    return zip.gcount();
}


size_t ZipReaderFile::read(std::vector<int8_t> &d) {
    return read(d.data(), d.size());
}





ZipEndOfCD readEndOfCD(std::ifstream &zip) {
	ZipEndOfCD endOfCd;
	zip.seekg(0, std::ios_base::end);
	int size = zip.tellg();

	auto pos = size - 4; // or so?
	while(pos>0) {
		zip.seekg(pos);
		if(zip.peek()==0x50) {
			// printf("found 0x50\n");
			char sig[4];
			zip.read(sig, 4);
			if(sig[1]==0x4b && sig[2]==0x05 && sig[3]==0x06) {
				// printf("Found header\n");
				zip.seekg(pos);
				zip.read((char*)&endOfCd, sizeof(ZipEndOfCD));
				zip.clear(); // clear eof
				break;
			}
		}
		pos--;
	}
	if(pos==0) {
		printf("Couldn't find header\n");
		throw std::runtime_error("Couldn't find end of cd header");
	}
	return endOfCd;
}



std::vector<ZipReader::Entry> readCD(std::ifstream &zip, const ZipEndOfCD &endOfCd) {
	std::vector<ZipReader::Entry> entries;
	std::vector<int8_t> cd(endOfCd.cdSizeBytes);
	if(!zip.good()) {
		printf("Zip stream broken\n");
		throw std::runtime_error("Zip stream not readable");
	}
	zip.seekg(endOfCd.cdFileOffset, std::ios_base::beg);
	zip.read((char*)cd.data(), cd.size());
	int charsRead = zip.gcount();
	if(charsRead!=cd.size()) {
		printf("Couldn't read first entry - only read %d chars(%ld)\n", charsRead, cd.size());
		throw std::runtime_error("Couldn't read first CD entry");
	}

	int offset = 0;
	while(offset<cd.size()) {
		ZipCDEntry *ent = (ZipCDEntry*)(cd.data() + offset);
		signed char *fileNamePtr = cd.data() + offset + 46;
		char fn[ent->fileNameLength+1];
		fn[ent->fileNameLength] = 0;
		memcpy(fn, fileNamePtr, ent->fileNameLength);
		
		entries.emplace_back(std::string(fn), ent->localHeaderOffset, ent->uncompressedSize);
		
		offset += ent->getTotalLength();
	}

	return entries;
}



ZipReader::ZipReader(const std::string &path) {
    this->zipPath = path;
    std::ifstream zip;
	zip.open(zipPath, std::ios::binary);
	if(!zip.good()) {
		printf("Can't read file");
		throw std::runtime_error("Can't read zip file '"+path+"'");
	}
	auto endOfCd = readEndOfCD(zip);
	entries = readCD(zip, endOfCd);
}


std::shared_ptr<ZipReaderFile> ZipReader::open(std::string pathInZip) {
	auto currEntry = findEntry(pathInZip);
	if(!currEntry.valid) return nullptr;
//	seekToEntry(currEntry);
	return std::make_shared<ZipReaderFile>(zipPath, currEntry);
	
}




//
//bool ZipReader::printTextFile(const std::string &path) {
//	auto entry = findEntry(path);
//	if(!entry.valid) return false;
//	printTextFile(entry);
//	return true;
//}

std::vector<std::string> ZipReader::list(bool print) {
	std::vector<std::string> ret;
	for(auto &s : entries) {
		ret.emplace_back(s.path);
		if(print) {
			printf("%s: %d/%d\n", s.path.c_str(), s.offset, s.size);
		}
	}
	return ret;
}

//
//void ZipReader::printTextFile(const Entry &e) {
//
//	seekToEntry(e);
//	std::vector<int8_t> d(e.size);
//	read(d);
//
//	for(auto c : d) {			
//		printf("%c", c);
//	}
//	printf("\n");
//}


//void ZipReader::seekToEntry(const Entry &e) {
//	zip.seekg(e.offset + 26);
//	int16_t fnExtraLength[2];
//	zip.read((char*)fnExtraLength, 4);
//
//	int fileStartOffset = e.offset + 30 + fnExtraLength[0] + fnExtraLength[1];
//
//	currEntryStart = fileStartOffset;
//	currEntryLength = e.size;
//	zip.seekg(fileStartOffset);
//}


ZipReader::Entry ZipReader::findEntry(const std::string &path) {
	for(const auto &e : entries) {
		if(e.path == path) {
			return e;
		}
	}
	return Entry();
}


