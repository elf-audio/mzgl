//
//  ZipStreamer.cpp
//  koala
//
//  Created by Marek Bereza on 18/10/2021.
//  Copyright © 2021 Marek Bereza. All rights reserved.
//

#include "ZipStreamer.h"
#include <stdio.h>
#include <minizip/unzip.h>

void ZipStreamer::close() {
	if (m_zf != NULL) {
		unzClose(m_zf);
		m_zf = NULL;
	}
}

bool ZipStreamer::open(const std::string &zipFile) {
#ifdef USEWIN32IOAPI
	zlib_filefunc64_def ffunc;
	fill_win32_filefunc64A(&ffunc);
	m_zf = unzOpen2_64(zipFile.c_str(), &ffunc);
#else
	m_zf = unzOpen64(zipFile.c_str());
#endif
	if (m_zf == NULL) return false;
	return true;
}
bool ZipStreamer::openFile(const std::string &pathInZip) {
	if (unzLocateFile(m_zf, pathInZip.c_str(), nullptr) != UNZ_OK) {
		printf("Failed to locate\n");
		return false;
	}

	int raw	   = 0;
	int method = 0;
	if (unzOpenCurrentFile2(m_zf, &method, nullptr, raw) != UNZ_OK) {
		printf("Failed to open current file\n");
		return false;
	}

	// if(unzOpenCurrentFile(m_zf)!=UNZ_OK) {
	// 	printf("Failed to open current file\n");
	// 	return false;
	// }
	return true;
}

ZipStreamer::~ZipStreamer() {
	close();
}

size_t ZipStreamer::read(std::vector<uint8_t> &d) {
	return (size_t) unzReadCurrentFile(m_zf, d.data(), (unsigned) d.size());
}

size_t ZipStreamer::read(void *d, size_t sz) {
	return (size_t) unzReadCurrentFile(m_zf, d, (unsigned) sz);
}
