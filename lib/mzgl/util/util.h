//
//  util.h
//  MZGL
//
//  Created by Marek Bereza on 15/01/2018.
//  Copyright © 2018 Marek Bereza. All rights reserved.
//

#pragma once

#include <string>
#include <functional>
#include <memory>
#include <vector>
#include <optional>
#include <algorithm>
#include <map>
#include <iterator>

class App;

void setThreadName(const std::string &name);

template <typename K, typename V>
std::optional<K> findKeyByValue(const std::map<K, V> &map, const V &value) {
	for (const auto &pair: map) {
		if (pair.second == value) {
			return pair.first; // Return the first key matching the value
		}
	}
	return {}; // Return an empty std::optional if the value is not found
}
std::string dataPath(const std::string &path, const std::string &bundleId = ""); //com.elf-audio.vst3.koala");
std::string docsPath(const std::string &path = "");
std::string appSupportPath(const std::string &path);
std::string getHomeDirectory();
void loadCommandLineArgs(int argc, const char *argv[]);
std::vector<std::string> getCommandLineArgs();
bool hasCommandLineFlag(const std::string &flag);
void addCommandLineFlag(const std::string &flag);
bool hasCommandLineSetting(const std::string &setting);
std::string getCommandLineSetting(const std::string &setting, const std::string &defaultValue = "");
int getCommandLineSetting(const std::string &setting, int defaultValue = -1);
std::string utf8decomposedToPrecomposed(const std::string &str);
std::string utf8precomposedToDecomposed(const std::string &str);
////////////////////////////////////////////////////////////////////////////////

template <typename T>
std::vector<T> concatenate(const std::vector<T> &source, const std::vector<T> &append) {
	std::vector<T> result = source;
	result.insert(result.end(), append.begin(), append.end());
	return result;
}

template <typename T>
void append(std::vector<T> &source, const std::vector<T> &append) {
	source.insert(source.end(), append.begin(), append.end());
}

/**
 * creates a new vector with elements of old vector that satisfy predicate.
 * e.g.
 * filterVector(std::vector<int>{-1, -2, 0, 3, 4}, [](int a) {return a>=0;});
 * this will return a vector with only positive numbers
 *
 * taken from here: https://www.cppstories.com/2021/filter-cpp-containers/
 */
template <typename T, typename Pred>
auto filterVector(const std::vector<T> &vec, Pred p) {
	std::vector<T> out;
	std::copy_if(begin(vec), end(vec), std::back_inserter(out), p);
	return out;
}
// from https://stackoverflow.com/questions/3424962/where-is-erase-if
// for std::vector
template <class T, class A, class Predicate>
void eraseIf(std::vector<T, A> &c, Predicate pred) {
	c.erase(std::remove_if(c.begin(), c.end(), pred), c.end());
}

template <class T, class Predicate>
[[maybe_unused]] bool eraseFirstIf(std::vector<T> &c, Predicate pred) {
	auto it = std::find_if(c.begin(), c.end(), pred);
	if (it != c.end()) {
		c.erase(it);
		return true;
	}
	return false;
}

#ifdef __APPLE__
void oslog(const std::string &s);
#endif

std::string getAppId();
float getSeconds();

bool copyDir(const std::string &source, const std::string &destination, std::string &errMsg);

// deletes a file, or moves it to the trash if implemented (it is on iOS and mac)
void deleteOrTrash(const std::string &path);

void setWindowSize(int w, int h);
// on iOS/mac this'll give the launched url
std::string getLaunchUrl();
// this is only for internal, it's so
// the app delegate can set this variable
// when loading the app
void setLaunchUrl(const std::string &url);

std::string tempDir();

std::string execute(std::string cmd, int *outExitCode = nullptr);

void sleepMillis(long ms);
void sleepMicros(long us);

// this makes sure a file path is unique by appending
// a number to the end of the filename (before the
// file extension)
std::string uniquerizePath(const std::string &path);

// only works on mac, this one.
void saveFileDialog(const std::string &msg,
					const std::string &defaultFileName,
					const std::vector<std::string> &allowedExtensions,
					std::function<void(std::string, bool)> completionCallback);

// generates a UUID - not tied to the machine
// but can be considered globally unique
std::string generateUUID();

void launchUrl(const std::string &url);

////////////////////////////////////////////////////////////////////////////////
/// Easy file writing

bool readFile(const std::string &filename, std::vector<unsigned char> &outData);
std::vector<unsigned char> readFile(const std::string &filename);
bool writeFile(const std::string &path, const std::vector<unsigned char> &data);
bool writeStringToFile(const std::string &path, const std::string &data);
bool writeStringToFileAtomically(const std::string &path, const std::string &data, std::string *err = nullptr);

bool readStringFromFile(const std::string &path, std::string &outStr);

////////////////////////////////////////////////////////////////////////////////
/// System info
std::string getOSVersion();
std::string getPlatformName();
std::string getAppVersionString();
uint64_t getStorageRemainingInBytes();

bool isTabletDevice();
// returns -1 if not supported
int64_t getAvailableMemory();

struct SafeInsets {
	int top;
	int right;
	int bottom;
	int left;
};
SafeInsets getSafeInsets();

////////////////////////////////////////////////////////////////////////////////

enum class Cursor {
	ARROW,
	IBEAM,
	CROSSHAIR,
	OPEN_HAND,
	CLOSED_HAND,
	POINTING_HAND,
	LEFT_RESIZE,
	RIGHT_RESIZE,
	UP_RESIZE,
	DOWN_RESIZE,
	LEFT_RIGHT_RESIZE,
	UP_DOWN_RESIZE,
};

void setCursor(Cursor cursor);

void showMouse();
void hideMouse();

////////////////////////////////////////////////////////////////////////////////
///// internal! Should definitely go elsewhere
void initMZGL(std::shared_ptr<App> app);

void setDocsPath(const std::string &newDocsPath);
void setDataPath(const std::string &newDataPath);

void quitApplication();

void writeToLockFile(const std::string &msg);
std::string readFromLockFile();

// the lockfile is a file that gets created when loading the auto saved state
// it is then deleted when reading is finished. If the lockfile exists when
// koala is trying to load the auto saved state, koala will give up trying to
// load it because it may have crashed koala on startup, and it will alert
// the user that there is a problem with the song. Therefore, it is important
// to delete the lockfile in the case that koala didn't crash, but hadn't
// finished loading the song before the user closes the app. So call
// deleteLockFileIfExists() on App::exit() because if App::exit() is called
// it means koala is exiting normally and didn't crash.
// if it returns true, don't try to auto save because there wasn't time to load the song.
bool deleteLockFileIfExists();
std::string getLockFilePath();
void disableLockFileWriting();
void enableLockFileWriting();