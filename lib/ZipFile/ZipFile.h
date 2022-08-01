//
//  ZipFile.h
//  koala
//
//  Created by Marek Bereza on 06/01/2020.
//  Copyright © 2020 Marek Bereza. All rights reserved.
//

#pragma once
#include <string>
#include <vector>

#include "filesystem.h"


class ZipFile {
public:
	enum class Compression {
		NONE,
		FASTER,
		SMALLER
	};
	// compression is 0, 1.
	static bool zip(const fs::path &dirToZip, const fs::path &outZipFile, Compression compression = Compression::SMALLER);
	static bool zip(const fs::path &dirToZip, std::vector<unsigned char> &outZipData, Compression compression = Compression::SMALLER);
	static bool unzip(const std::vector<unsigned char> &inZipData, const fs::path &outDir);
	static bool unzip(const fs::path &zipFile, const fs::path &outDir);
	static void listZip(const fs::path &pathToZip, std::vector<std::string> &fileList);

	static bool getTextFileFromZip(const fs::path &pathToZip, const fs::path &filePath, std::string &outData);
	static bool getBinaryFileFromZip(const fs::path &pathToZip, const fs::path &filePath, std::vector<unsigned char> &data);
};
