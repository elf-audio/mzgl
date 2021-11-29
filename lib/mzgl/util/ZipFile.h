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


namespace boost { namespace filesystem {
	class path;
}};



class ZipFile {
public:
	enum class Compression {
		NONE,
		FASTER,
		SMALLER
	};
	// compression is 0, 1.
	static bool zip(const boost::filesystem::path &dirToZip, const boost::filesystem::path &outZipFile, Compression compression = Compression::SMALLER);
	static bool zip(const boost::filesystem::path &dirToZip, std::vector<unsigned char> &outZipData, Compression compression = Compression::SMALLER);
	static bool unzip(const std::vector<unsigned char> &inZipData, const boost::filesystem::path &outDir);
	static bool unzip(const boost::filesystem::path &zipFile, const boost::filesystem::path &outDir);
	static void listZip(const boost::filesystem::path &pathToZip, std::vector<std::string> &fileList);
	static bool getTextFileFromZip(const boost::filesystem::path &pathToZip, const boost::filesystem::path &filePath, std::string &outData);
	static bool getBinaryFileFromZip(const boost::filesystem::path &pathToZip, const boost::filesystem::path &filePath, std::vector<unsigned char> &data);

};
