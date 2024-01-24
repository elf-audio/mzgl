//
//  ZipFile.m
//  Koala Sampler
//
//  Created by Marek Bereza on 06/01/2020.
//  Copyright © 2020 Marek Bereza. All rights reserved.
//

#include "ZipFile.h"
#include <zipper/zipper.h>
#include <zipper/unzipper.h>

zipper::Zipper::zipFlags ZipFileFlagsToZipperFlags(ZipFile::Compression compression) {
	switch (compression) {
		case ZipFile::Compression::SMALLER: return zipper::Zipper::zipFlags::Better;
		case ZipFile::Compression::FASTER: return zipper::Zipper::zipFlags::Faster;
		case ZipFile::Compression::NONE: return zipper::Zipper::zipFlags::Store;
	}
	return zipper::Zipper::zipFlags::Better;
}

bool addFilesToZipAndClose(zipper::Zipper &zipper, const fs::path &dirToZip, ZipFile::Compression compression) {
	auto flags = ZipFileFlagsToZipperFlags(compression);
	for (auto &p: fs::directory_iterator(dirToZip)) {
		zipper.add(p.path().string(), flags);
	}
	zipper.close();
	return true;
}

bool ZipFile::zip(const fs::path &dirToZip, const fs::path &outZipFile, Compression compression) {
	zipper::Zipper zipper(outZipFile.string());
	return addFilesToZipAndClose(zipper, dirToZip, compression);
}

bool ZipFile::zip(const fs::path &dirToZip, std::vector<uint8_t> &outZipData, Compression compression) {
	zipper::Zipper zipper(outZipData);
	return addFilesToZipAndClose(zipper, dirToZip, compression);
}

bool ZipFile::unzip(const fs::path &zipFile, const fs::path &outDir) {
	zipper::Unzipper unzipper(zipFile.string());
	return unzipper.extract(outDir.string());
}

bool ZipFile::unzip(const std::vector<uint8_t> &inZipData, const fs::path &outDir) {
	// have to unconst it here, not cool but it's not my library
	zipper::Unzipper unzipper((std::vector<unsigned char> &) inZipData);
	return unzipper.extract(outDir.string());
}

void ZipFile::listZip(const fs::path &pathToZip, std::vector<std::string> &fileList) {
	zipper::Unzipper unzipper(pathToZip.string());
	fileList.clear();
	for (const auto &e: unzipper.entries()) {
		fileList.emplace_back(e.name);
	}
}
// throws a runtime error if there was a problem
bool ZipFile::getTextFileFromZip(const fs::path &pathToZip, const fs::path &filePath, std::string &outData) {
	try {
		zipper::Unzipper unzipper(pathToZip.string());
		std::vector<unsigned char> vec;
		if (unzipper.extractEntryToMemory(filePath.string(), vec)) {
			outData.assign(reinterpret_cast<char *>(&vec[0]), vec.size());
			return true;
		} else {
			return false;
		}
	} catch (const std::runtime_error &err) {
		fprintf(stderr, "Got error trying to getTextFileFromZip() %s", err.what());
		return false;
	}
}

bool ZipFile::getBinaryFileFromZip(const fs::path &pathToZip,
								   const fs::path &filePath,
								   std::vector<uint8_t> &data) {
	return zipper::Unzipper {pathToZip.string()}.extractEntryToMemory(filePath.string(), data);
}
