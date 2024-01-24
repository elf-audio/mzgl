#pragma once

#include "defs.h"
#include <ctime>
#include <chrono>
#include <fsystem/fsystem.h>
#include <sys/types.h>
#include <sys/stat.h>
#if defined(USE_WINDOWS)
#    include <Windows.h>
#endif

/**
 * @brief Creates a timestap either from file or just current time
 * If it fails to read the timestamp of a file, it set the time stamp to current time
 * 
 * @warning It uses std::time to get current time, which is not standardized to be 1970-01-01....
 * However, it works on Windows and Unix https://stackoverflow.com/questions/6012663/get-unix-timestamp-with-c 
 * With C++20 this will be standardized
 */
struct Timestamp
{
    tm timestamp;
    Timestamp();
    Timestamp(const std::string& filepath);
    Timestamp(const fs::path& filepath);
};

static void init(const fs::path& filepath, tm &timestamp)
{
    STAT st;
#if defined(USE_WINDOWS)
    _wstat(filepath.c_str(), &st);
#else
    stat(filepath.c_str(), &st);
#endif

    auto timet = static_cast<time_t>(st.st_mtime);
    timestamp = *std::localtime(&timet);
}


Timestamp::Timestamp()
{
    std::time_t now = std::time(nullptr);
    timestamp = *std::localtime(&now);
}

Timestamp::Timestamp(const fs::path& filepath)
{
    init(filepath, timestamp);
}

Timestamp::Timestamp(const std::string& filepath)
{
    init(fs::u8path(filepath.c_str()), timestamp);
}
