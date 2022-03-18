//
//  ZipStreamer.cpp
//  koala
//
//  Created by Marek Bereza on 18/10/2021.
//  Copyright © 2021 Marek Bereza. All rights reserved.
//

#include "ZipStreamer.h"
#include <stdio.h>
#include "zipper/minizip/ioapi_mem.h"
#include "zipper/minizip/unzip.h"



void ZipStreamer::close() {
	if (m_zf != NULL) {
		unzClose(m_zf);
		m_zf = NULL;
	}
}

bool ZipStreamer::open(const std::string& filename) {
#ifdef USEWIN32IOAPI
	zlib_filefunc64_def ffunc;
	fill_win32_filefunc64A(&ffunc);
	m_zf = unzOpen2_64(filename.c_str(), &ffunc);
#else
	m_zf = unzOpen64(filename.c_str());
#endif
	return m_zf != NULL;
}

ZipStreamer::~ZipStreamer() {
	close();
}

bool ZipStreamer::openFile(const std::string &path) {
	if(unzLocateFile(m_zf, path.c_str(), nullptr)!=UNZ_OK) {
		printf("Failed to locate\n");
		return false;
	}
	if(unzOpenCurrentFile(m_zf)!=UNZ_OK) {
		printf("Failed to open current file\n");
		return false;
	}
	return true;
}

size_t ZipStreamer::read(std::vector<uint8_t> &d) {
	return (size_t)unzReadCurrentFile(m_zf, d.data(), (unsigned)d.size());
}

size_t ZipStreamer::read(void *d, size_t sz) {
	return (size_t)unzReadCurrentFile(m_zf, d, (unsigned)sz);
}
