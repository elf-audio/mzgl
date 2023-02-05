#pragma once

#include <string>
#include <vector>
#include <istream>
#include "filesystem.h"

namespace zipper {

// TODO: replace std::string with fs::path

void getFileCrc(std::istream& input_stream, std::vector<char>& buff, unsigned long& result_crc);
bool isLargeFile(std::istream& input_stream);
bool checkFileExists(const std::string& filename);
bool makedir(const std::string& newdir);
void removeFolder(const fs::path& foldername);
std::string parentDirectory(const std::string& filepath);
std::string currentPath();
bool isDirectory(const fs::path& foldername);
std::vector<fs::path> filesFromDirectory(const std::string& path);

std::string fileNameFromPath(const std::string& path);

} // namespace zipper
