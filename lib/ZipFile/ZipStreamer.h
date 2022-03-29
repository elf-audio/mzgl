//
//  ZipStreamer.h
//  koala
//
//  Created by Marek Bereza on 18/10/2021.
//  Copyright © 2021 Marek Bereza. All rights reserved.
//

#pragma once
#include <vector>
#include <string>
#include <memory>
#include "zipper/minizip/zip.h"


/**
 * This class is the old way or reading from zip files - it can't seek - ZipReader is the new version.
 * Whilst minizip appears to let you seek, I couldn't get it to work properly, so ZipReader is
 * my own implementation.
 */
struct ZipStreamer {

	enum class SeekOrigin {
		Start,
		Current
	};
	bool open(const std::string& zipFile, const std::string &pathInZip);

	size_t read(std::vector<uint8_t> &d);	
	size_t read(void *d, size_t sz);


	void close();
	~ZipStreamer();
private:
	zipFile m_zf;
};

