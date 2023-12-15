#include "FileWatcher.h"

#ifndef _WIN32
#	include <sys/stat.h>
#	include <unistd.h>
#endif
using namespace std;

void FileWatcher::watch(const std::string &path) {
	// this checks to see if the file has already been added, and only
	// adds if it's not already here.
	// should really be a set but can't modify when iterating
	for (auto &v: watchedFiles) {
		if (v.path == path) {
			printf("Already has %s\n", path.c_str());
			return;
		}
	}
	watchedFiles.emplace_back(path);
	printf("Added %s\n", path.c_str());
}

void FileWatcher::tick() {
	if (watchedFiles.empty()) return;
	waitCount++;
	if (waitCount < 30) {
		return;
	}
	waitCount = 0;

	bool hasBeenTouched = false;
	for (auto &file: watchedFiles) {
		struct stat fileStat;
		if (stat(file.path.c_str(), &fileStat) < 0) {
			printf("Couldn't find file %s\n", file.path.c_str());
			return;
		}
		long currUpdateTime = fileStat.st_mtime;
		if (currUpdateTime != file.prevUpdateTime) {
			hasBeenTouched = true;
		}
		file.prevUpdateTime = currUpdateTime;
	}

	if (hasBeenTouched && touched != nullptr) touched();
}
