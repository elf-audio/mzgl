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
#include <glm/glm.hpp>

class App;

std::string dataPath(std::string path, std::string bundleId = "");
std::string docsPath(std::string path = "");
std::string appSupportPath(std::string path);

#ifdef __APPLE__
void oslog(std::string s);
#endif
std::string getAppId();
float getSeconds();
unsigned int getFrameNum();

bool copyDir(const std::string &source, const std::string &destination, std::string &errMsg);

std::string getHomeDirectory();

void setWindowSize(int w, int h);
void setWindowTitle(std::string title);

extern std::vector<std::string> commandLineArgs;

uint64_t getStorageRemainingInBytes();
std::string byteSizeToString(uint64_t bytes);

// on iOS this'll give the launched url
std::string getLaunchUrl();

std::string tempDir();
// this is only for internal, it's so
// the app delegate can set this variable
// when loading the app
void setLaunchUrl(std::string url);

std::string execute(std::string cmd, int *outExitCode = nullptr);
///// internal!!
void setSize(int w, int h);
class Graphics;
class EventDispatcher;
void initMZGL(App *app);
void drawFrame(Graphics &g, EventDispatcher *eventDispatcher);
void updateInternal();
void loadCommandLineArgs(int argc, const char *argv[]);
std::vector<std::string> getCommandLineArgs();

void sleepMillis(long ms);
// this makes sure a file path is unique by appending
// a number to the end of the filename (before the
// file extension) 
std::string uniquerizePath(std::string path);

// from oF
//--------------------------------------------------
std::vector <std::string> split(const std::string & source, const std::string & delimiter, bool ignoreEmpty = false, bool trim = false);
//--------------------------------------------------
std::string trimFront(const std::string & src);

//--------------------------------------------------
std::string trimBack(const std::string & src);

template <typename T>
std::string to_string(const T a_value, const int n);

//--------------------------------------------------
std::string trim(const std::string & src);

glm::vec2 getMousePosition();
bool isTouchDown(int which = 0);

std::string to_string(float value, int precision);

std::string to_string(double value, int precision);

std::string tolower(std::string s);
void replaceAll(std::string & data, std::string toSearch, std::string replaceStr);

// only works on mac, this one.
void saveFileDialog(std::string msg, std::string defaultFileName, std::function<void(std::string, bool)> completionCallback);

// only works on mac, this one.
void loadFileDialog(std::string msg, std::function<void(std::string, bool)> completionCallback);
void loadFileDialog(std::string msg, const std::vector<std::string> &allowedExts, std::function<void(std::string, bool)> completionCallback);

//void alertDialog(string title, string msg);

// this function makes a copy of the loaded image so be sure to delete the original file if it exists
//void chooseImageDialog(std::function<void(bool success, string imgPath)>);


// generates a UUID - not tied to the machine
// but can be considered globally unique
std::string generateUUID();

// only works on iOS. this one
//void shareDialog(std::string message, std::string path, std::function<void(bool)> completionCallback);

// only works on iOS. this one
void launchUrl(std::string url);

// also only in iOS
//void launchUrlInWebView(std::string url, std::function<void()> completionCallback);

void showMouse();
void hideMouse();

std::string getAppVersionString();

bool isMainThread();

void runOnMainThread(std::function<void()> fn);

// if the call to this is made on the main thread, this just runs it
// rather than enqueueing it on the mainThreadQueue
void runOnMainThread(bool checkIfOnMainThread, std::function<void()> fn);

void runOnMainThreadAndWait(std::function<void()> fn);

// only use this if you want to change which 
// thread is marked as the "main" thread - 
// all you have to do is call it on the thread
// you want to set.
void setMainThreadId();


void runTask(std::function<void()> fn);

bool readFile(std::string filename, std::vector<unsigned char> &outData);
std::vector<unsigned char> readFile(std::string filename);
bool writeFile(const std::string &path, const std::vector<unsigned char> &data);


bool writeStringToFile(const std::string &path, const std::string &data);
bool readStringFromFile(const std::string &path, std::string &outStr);
std::string getOSVersion();
std::string getPlatformName();
#include <assert.h>
#if DEBUG==1
#include "log.h"

#define mzAssert(A) if(mzAssertEnabled()) { bool a = (A); if(!a) { Log::e() << "ASSERTION FAILED IN " << __FILE__ << " at line "<< __LINE__;} assert(a);}
#else
#define mzAssert(A) {};
#endif

void mzEnableAssert(bool enabled);
bool mzAssertEnabled();

class MZScopedAssertDisable {
public:
	MZScopedAssertDisable() {
		mzEnableAssert(false);
	}
	
	~MZScopedAssertDisable() {
		mzEnableAssert(true);
	}
};

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

#ifdef UNIT_TEST

void setDocsPath(std::string newDocsPath);
void setDataPath(std::string newDataPath);

// dangerous to use in anything but testing
void clearMainThreadQueue();
void waitTilAllTasksAreDone();
#endif
