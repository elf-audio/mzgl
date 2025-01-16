#pragma once

#include "filesystem.h"

#include <functional>
#include <string>

struct FileDownloaderCallbacks {
	std::function<void()> downloadStarting;
	std::function<void(const fs::path &)> downloadCompleted;
	std::function<void(const std::string &)> downloadFailed;
	std::function<void()> downloadCancelled;
	std::function<void(float)> downloadProgressChanged;
};

class FileDownloader {
public:
	FileDownloader() = default;
	~FileDownloader();

	void download(const std::string &url,
				  const fs::path &downloadLocation,
				  const FileDownloaderCallbacks &callbacks);
	void cancel();

private:
	void *platformHandle {nullptr};
};
