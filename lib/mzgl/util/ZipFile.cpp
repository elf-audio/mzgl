//
//  ZipFile.m
//  Koala Sampler
//
//  Created by Marek Bereza on 06/01/2020.
//  Copyright © 2020 Marek Bereza. All rights reserved.
//

#include "ZipFile.h"
#include "zipper.h"
#include "unzipper.h"

using namespace zipper;

#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;

Zipper::zipFlags ZipFileFlagsToZipperFlags(ZipFile::Compression compression) {
	switch(compression) {
		case ZipFile::Compression::SMALLER: return Zipper::zipFlags::Better;
		case ZipFile::Compression::FASTER: return Zipper::zipFlags::Faster;
		case ZipFile::Compression::NONE: return Zipper::zipFlags::Store;
	}
	return Zipper::zipFlags::Better;
}

//	auto flags = ZipFileFlagsToZipperFlags(compression);
bool addFilesToZipAndClose(Zipper &zipper, const fs::path &dirToZip, ZipFile::Compression compression) {
	auto flags = ZipFileFlagsToZipperFlags(compression);
	for(auto &p : fs::directory_iterator(dirToZip)) {
		zipper.add(p.path().string(), flags);
	}
	zipper.close();
	return true;
}


bool ZipFile::zip(const fs::path &dirToZip, const fs::path &outZipFile, Compression compression) {
	
	Zipper zipper(outZipFile.string());
	return addFilesToZipAndClose(zipper, dirToZip, compression);
}


bool ZipFile::zip(const fs::path &dirToZip, std::vector<unsigned char> &outZipData, Compression compression) {
	Zipper zipper(outZipData);
	return addFilesToZipAndClose(zipper, dirToZip, compression);
}

bool ZipFile::unzip(const fs::path &zipFile, const fs::path &outDir) {
	Unzipper unzipper(zipFile.string());
	return unzipper.extract(outDir.string());
}

bool ZipFile::unzip(const std::vector<unsigned char> &inZipData, const fs::path &outDir) {
	// have to unconst it here, not cool but it's not my library
	Unzipper unzipper((std::vector<unsigned char> &)inZipData);
	return unzipper.extract(outDir.string());
}

void ZipFile::listZip(const fs::path &pathToZip, std::vector<std::string> &fileList) {
	Unzipper unzipper(pathToZip.string());
	auto entries = unzipper.entries();
	fileList.clear();
	for(const auto &e : entries) {
//		printf("%s\n", e.name.c_str());
		fileList.emplace_back(e.name);
	}
}
 
bool ZipFile::getTextFileFromZip(const fs::path &pathToZip, const fs::path &filePath, std::string &outData) {
	Unzipper unzipper(pathToZip.string());
	std::vector<unsigned char> vec;
	if(unzipper.extractEntryToMemory(filePath.string(), vec)) {
		outData.assign(reinterpret_cast<char*>(&vec[0]), vec.size());
		return true;
	} else {
		return false;
	}

}

bool ZipFile::getBinaryFileFromZip(const fs::path &pathToZip, const fs::path &filePath, std::vector<unsigned char> &data) {
	Unzipper unzipper(pathToZip.string());
	
	if(unzipper.extractEntryToMemory(filePath.string(), data)) {
		return true;
	} else {
		return false;
	}
}
