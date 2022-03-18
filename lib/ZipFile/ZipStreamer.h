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


struct ZipStreamer {

	bool open(const std::string& filename);
	
	bool openFile(const std::string &path);
	
	size_t read(std::vector<uint8_t> &d);
	
	size_t read(void *d, size_t sz);
	
	void close();
	~ZipStreamer();
private:
	zipFile m_zf;
};

