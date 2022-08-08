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
#include <vector>

class App;

std::string dataPath(std::string path, std::string bundleId = "");//com.elf-audio.vst3.koala");
std::string docsPath(std::string path = "");
std::string appSupportPath(std::string path);
std::string getHomeDirectory();
std::string getCWD();
void loadCommandLineArgs(int argc, const char *argv[]);
std::vector<std::string> getCommandLineArgs();


#ifdef __APPLE__
void oslog(std::string s);
#endif

std::string getAppId();
float getSeconds();
unsigned int getFrameNum();

bool copyDir(const std::string &source, const std::string &destination, std::string &errMsg);


void setWindowSize(int w, int h);
void setWindowTitle(std::string title);

extern std::vector<std::string> commandLineArgs;

// on iOS this'll give the launched url
std::string getLaunchUrl();

std::string tempDir();
// this is only for internal, it's so
// the app delegate can set this variable
// when loading the app
void setLaunchUrl(std::string url);

std::string execute(std::string cmd, int *outExitCode = nullptr);


void sleepMillis(long ms);
// this makes sure a file path is unique by appending
// a number to the end of the filename (before the
// file extension) 
std::string uniquerizePath(std::string path);


// only works on mac, this one.
void saveFileDialog(std::string msg, std::string defaultFileName, std::function<void(std::string, bool)> completionCallback);


// generates a UUID - not tied to the machine
// but can be considered globally unique
std::string generateUUID();

// only works on iOS. this one
void launchUrl(std::string url);



////////////////////////////////////////////////////////////////////////////////
/// Easy file writing

bool readFile(std::string filename, std::vector<unsigned char> &outData);
std::vector<unsigned char> readFile(std::string filename);
bool writeFile(const std::string &path, const std::vector<unsigned char> &data);
bool writeStringToFile(const std::string &path, const std::string &data);
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
void initMZGL(App *app);
void updateInternal();

#ifdef UNIT_TEST

void setDocsPath(std::string newDocsPath);
void setDataPath(std::string newDataPath);

#endif
