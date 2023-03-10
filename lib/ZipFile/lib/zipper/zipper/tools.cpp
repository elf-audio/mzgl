#include "tools.h"
#include "defs.h"
#include "minizip/ioapi_mem.h"
#include <algorithm>
#include <iterator>

#include "CDirEntry.h"

#include <cstdio>
#include <iostream>

#if defined(USE_WINDOWS)
#    include "tps/dirent.h"
#    include "tps/dirent.c"
#else
#    include <sys/types.h>
#    include <dirent.h>
#    include <unistd.h>
#endif /* WINDOWS */

namespace zipper {

// -----------------------------------------------------------------------------
// Calculate the CRC32 of a file because to encrypt a file, we need known the
// CRC32 of the file before.
void getFileCrc(std::istream& input_stream, std::vector<char>& buff, unsigned long& result_crc)
{
    unsigned long calculate_crc = 0;
    unsigned int size_read = 0;
    unsigned long total_read = 0;

    do
    {
        input_stream.read(buff.data(), std::streamsize(buff.size()));
        size_read = static_cast<unsigned int>(input_stream.gcount());

        if (size_read > 0)
            calculate_crc = crc32(calculate_crc, reinterpret_cast<const unsigned char*>(buff.data()), size_read);

        total_read += static_cast<unsigned long>(size_read);

    } while (size_read > 0);

    input_stream.clear();
    input_stream.seekg(0, std::ios_base::beg);
    result_crc = calculate_crc;
}

// -----------------------------------------------------------------------------
bool isLargeFile(std::istream& input_stream)
{
    std::streampos pos = 0;
    input_stream.seekg(0, std::ios::end);
    pos = input_stream.tellg();
    input_stream.seekg(0);

    return pos >= 0xffffffff;
}

// -----------------------------------------------------------------------------
bool checkFileExists(const std::string& filename)
{
    return CDirEntry::exist(filename);
}

// -----------------------------------------------------------------------------
bool makedir(const std::string& newdir)
{
    return CDirEntry::createDir(newdir);
}

// -----------------------------------------------------------------------------
void removeFolder(const fs::path& foldername)
{
    if (!CDirEntry::remove(foldername.string()))
    {
        std::vector<fs::path> files = filesFromDirectory(foldername.string());
        std::vector<fs::path>::iterator it = files.begin();
        for (; it != files.end(); ++it)
        {
            if (isDirectory(*it) && *it != foldername)
                removeFolder(*it);
            else
                fs::remove(*it);
        }
        CDirEntry::remove(foldername.string());
    }
}

// -----------------------------------------------------------------------------
bool isDirectory(const fs::path& path)
{
    return CDirEntry::isDir(path.string());
}

// -----------------------------------------------------------------------------
std::string parentDirectory(const std::string& filepath)
{
    return CDirEntry::dirName(filepath);
}

// -----------------------------------------------------------------------------
std::string currentPath()
{
    char buffer[1024u];
    return (getcwd(buffer, sizeof(buffer)) ? std::string(buffer) : std::string(""));
}

// -----------------------------------------------------------------------------
std::vector<fs::path> filesFromDirectory(const std::string& path)
{
    std::vector<fs::path> dir_ls;

    for (auto& dir_entry : fs::recursive_directory_iterator(fs::u8path(path)))
    {
        if (!isDirectory(dir_entry.path()))
        {
			dir_ls.push_back(dir_entry.path());
		}
    }
    return dir_ls;
}


// -----------------------------------------------------------------------------
std::string fileNameFromPath(const std::string& fullPath)
{
    return CDirEntry::fileName(fullPath);
}

} // namespace zipper
