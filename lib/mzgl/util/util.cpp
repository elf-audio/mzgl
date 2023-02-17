//
//  util.cpp
//  MZGL
//
//  Created by Marek Bereza on 15/01/2018.
//  Copyright © 2018 Marek Bereza. All rights reserved.
//

#include "util.h"
#include <algorithm>
#include <cctype>
#include <iterator>

#include "mzgl_platform.h"
#ifdef __APPLE__
#	include <os/log.h>
#	include <TargetConditionals.h>
#	include <Foundation/Foundation.h>
#	if TARGET_OS_IOS
#		import <UIKit/UIKit.h>
#	else
#		include <Cocoa/Cocoa.h>
#	endif

#endif
#ifdef __ANDROID__
#	include "androidUtil.h"
#	include "koalaAndroidUtil.h"
#elif defined(__linux__)
#	include "linuxUtil.h"
#endif

#ifdef _WIN32
#	include "winUtil.h"
#	include <windows.h>
#	include <winuser.h>
#	include <commdlg.h>
#	include <direct.h>
#	define _WIN32_DCOM

#	include <shlobj.h>
#	include <tchar.h>
#	include <stdio.h>
#	include <ShObjIdl_core.h>
#	include <locale>
#	include <codecvt>

#endif

#include "Graphics.h"
#include <chrono>
#include <fstream>

#include "log.h"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include "Graphics.h"

#include "filesystem.h"

#include "App.h"
#include "mainThread.h"
#include <thread>
using namespace std;

// this from openframeworks
#ifdef _WIN32
#	include <locale>
#	include <sstream>
#	include <string>

#endif

#include <stdlib.h>
#include <stdio.h>

#ifndef _WIN32
#	include <pwd.h>
#	include <unistd.h>

#	ifdef __APPLE__
std::string getHomeDirectory() {
	return [NSHomeDirectory() UTF8String];
}
#	else
std::string getHomeDirectory() {
	const char *homeDir = getenv("HOME");

	if (!homeDir) {
		struct passwd *pwd = getpwuid(getuid());
		if (pwd) homeDir = pwd->pw_dir;
	}
	return homeDir;
}
#	endif
#endif

#ifdef _WIN32
std::string getCWD() {
	char c[512];
	_getcwd(c, 512);
	return c;
}
#else
std::string getCWD() {
	char c[512];
	getcwd(c, 512);
	return c;
}
#endif

bool copyDir(const fs::path &source, const fs::path &destination, string &errMsg) {
	try {
		// Check whether the function call is valid
		if (!fs::exists(source) || !fs::is_directory(source)) {
			errMsg = "Source directory " + source.string() + " does not exist or is not a directory.";
			return false;
		}

		if (fs::exists(destination)) {
			errMsg = "Destination directory " + destination.string() + " already exists.";
			return false;
		}

		// Create the destination directory
		if (!fs::create_directory(destination)) {
			errMsg = "Unable to create destination directory" + destination.string();
			return false;
		}
	} catch (fs::filesystem_error const &e) {
		errMsg = string(e.what());
		return false;
	}

	// Iterate through the source directory
	for (fs::directory_iterator file(source); file != fs::directory_iterator(); ++file) {
		try {
			fs::path current(file->path());
			if (fs::is_directory(current)) {
				// Found directory: Recursion
				if (!copyDir(current, destination / current.filename(), errMsg)) return false;

			} else {
				// Found file: Copy
				fs::copy_file(current, destination / current.filename());
			}
		} catch (fs::filesystem_error const &e) {
			errMsg = string(e.what());
			return false;
		}
	}
	return true;
}

bool copyDir(const std::string &source, const std::string &destination, string &errMsg) {
	return copyDir(fs::path(source), fs::path(destination), errMsg);
}

void sleepMillis(long ms) {
	std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

bool is_number(const std::string &s) {
	return !s.empty() && std::find_if(s.begin(), s.end(), [](char c) { return !std::isdigit(c); }) == s.end();
}

string uniquerizePath(string path) {
	fs::path p(path);
	string outPath = p.string();
	if (fs::exists(p)) {
		string filenameBase = p.stem().string();
		int lastSlash = (int) filenameBase.rfind("-");

		int startingIndex = 1;
		if (lastSlash != -1) {
			string ending = filenameBase.substr(lastSlash + 1);
			if (is_number(ending)) {
				filenameBase = filenameBase.substr(0, lastSlash);
				startingIndex = stoi(ending) + 1;
			}
		}

		for (int i = startingIndex; i < 10000; i++) {
			auto filename = p.stem();
			auto pp = p.parent_path() / (filenameBase + "-" + to_string(i) + p.extension().string());

			if (!fs::exists(pp)) {
				outPath = pp.string();
				break;
			}
		}
		if (fs::exists(outPath)) {
			Log::e() << "ERROR: can't make unique file name, ran out of numbers!!";
		}
	}
	return outPath;
}

vector<string> commandLineArgs;
void loadCommandLineArgs(int argc, const char *argv[]) {
	commandLineArgs.clear();
	for (int i = 0; i < argc; i++) {
		commandLineArgs.push_back(argv[i]);
	}
}

std::vector<std::string> getCommandLineArgs() {
	return commandLineArgs;
}

namespace Globals {
string launchUrl = "";
std::chrono::system_clock::time_point startTime;
} // namespace Globals

float getSeconds() {
	auto end = std::chrono::system_clock::now();
	std::chrono::duration<double> elapsed_seconds = end - Globals::startTime;
	return elapsed_seconds.count();
}

#ifdef __APPLE__
#	if !TARGET_OS_IOS
#		include "MacAppDelegate.h"
#		include "EventsView.h"
#	endif
#endif

void setWindowSize(int w, int h) {
#ifdef __APPLE__
#	if TARGET_OS_IOS
	// do nothing. iPhones have fixed window size
#	else
	NSWindow *win = [NSApp mainWindow];
	//	win.frame.size = CGSizeMake(2000, 2000);

	// these lines were commented out because we don't have SCALE_FACTOR any more.
	// it's now g.pixelScale
	//	w /= g.pixelScale;
	//	h /= g.pixelScale;

	NSRect frame = win.frame;
	NSSize newSize = CGSizeMake(w, h);

	frame.origin.y -= frame.size.height;
	frame.origin.y += newSize.height;
	frame.size = newSize;
	dispatch_async(dispatch_get_main_queue(), ^(void) {
	  [win setFrame:frame display:YES animate:NO];
	  //		// get window delegate which is events view
	  //		EventsView *delegate = (EventsView*)win.delegate;
	  //		// then call windowDidEndLiveResize:(NSNotification *)notification
	  //		NSNotification *notif = [[NSNotification alloc] initWithName:@"" object:win userInfo:nil];
	  //		[delegate windowDidEndLiveResize:notif];
	});

#	endif
#endif
}

void setWindowTitle(string title) {
#if defined(__APPLE__) && !TARGET_OS_IOS

	dispatch_async(dispatch_get_main_queue(), ^(void) {
	  NSWindow *win = [NSApp mainWindow];
	  [win setTitle:[NSString stringWithUTF8String:title.c_str()]];
	});
#endif
}

string tempDir() {
#if defined(__APPLE__) // && TARGET_OS_IOS
	string _p;
	if (@available(macOS 10.12, *)) {
		NSURL *url = [[NSFileManager defaultManager] temporaryDirectory];
		_p = [[url path] UTF8String];
	} else {
		// Fallback on earlier versions
		_p = [NSTemporaryDirectory() UTF8String];
	}

//	string _p = [NSTemporaryDirectory() UTF8String];
#elif defined(__ANDROID__)
	string _p = getAndroidTempDir();
#else
	string _p = "/tmp";
#endif
	return _p;
}

#ifdef UNIT_TEST
bool isOverridingDataPath = false;
string dataPathOverride = "";

void setDataPath(string path) {
	isOverridingDataPath = true;
	dataPathOverride = path;
}
#endif

// TODO: we would have an option for mac to load from its bundle rather than the dataPath (i.e. mac and iOS use same code)
string dataPath(string path, string appBundleId) {
	// it's an absolute path, don't do anything to it
	if (path.size() > 0 && path[0] == '/') return path;

#ifdef UNIT_TEST
	if (isOverridingDataPath) return dataPathOverride + "/" + path;
#endif

#if defined(__APPLE__) && defined(MZGL_MAC_GLFW)

	return "data/" + path;
#elif defined(__APPLE__)

	if (appBundleId == "") {
		//		Log::e() << "Going for main bundle";
		NSString *a = [[NSBundle mainBundle] resourcePath];
		string s = [a UTF8String];
		s += "/data/" + path;
		//		Log::e() << s;
		return s;
	} else {
		Log::e() << "Going for bundle" << appBundleId;
		NSBundle *pBundle = [NSBundle bundleWithIdentifier:[NSString stringWithCString:appBundleId.c_str() encoding:NSUTF8StringEncoding]];
		if (pBundle == nil) {
			return path;
		} else {
			string returnPath = string([[pBundle resourcePath] UTF8String]) + "/data/" + path;
			Log::e() << "final path: " << returnPath;

			return returnPath;
		}
	}
#elif defined(__RPI)
	return "../data/" + path;
#elif defined(_WIN32)
	return "../data/" + path;
#else
	return "../data/" + path;
#endif
}

#ifdef UNIT_TEST
bool isOverridingDocsPath = false;
string docsPathOverride = "";

void setDocsPath(string path) {
	isOverridingDocsPath = true;
	docsPathOverride = path;
}
#endif

#ifdef __APPLE__
#	include <os/proc.h>
#endif
int64_t getAvailableMemory() {
#ifdef __APPLE__
#	if TARGET_OS_IOS
	if (@available(iOS 13.0, *)) {
		return os_proc_available_memory();
	} else {
		return -1;
	}
#	else
	return -1;
#	endif
#endif
#ifdef __ANDROID__
	return androidGetAvailableMemory();
#endif
	Log::e() << "Warning - getAvailableMemory() doesn't work on this OS";
	return 10000000000;
}
string docsPath(string path) {
#ifdef UNIT_TEST
	if (isOverridingDocsPath) {
		return docsPathOverride + "/" + path;
	}
#endif
#ifdef __APPLE__
	NSURL *url = [[[NSFileManager defaultManager] URLsForDirectory:NSDocumentDirectory inDomains:NSUserDomainMask] lastObject];
	string _path = [[url path] UTF8String];

	return _path + "/" + path;
#elif defined(__ANDROID__)
	return getAndroidExternalDataPath() + "/" + path;
#elif defined(_WIN32)

	wchar_t *documentsDir;
	HRESULT result = SHGetKnownFolderPath(FOLDERID_AppDataDocuments, KF_FLAG_CREATE | KF_FLAG_INIT, NULL, &documentsDir);
	string retPath;

	if (result == S_OK) {
		std::wstring ws(documentsDir);
		std::string documentsDirUtf8 = std::wstring_convert<std::codecvt_utf8<wchar_t>>().to_bytes(ws);

		TCHAR szExeFileName[MAX_PATH];
		GetModuleFileName(NULL, szExeFileName, MAX_PATH);
		retPath = documentsDirUtf8 + "\\" + fs::path(string(szExeFileName)).stem().string();
		if (!fs::exists(retPath)) {
			fs::create_directory(retPath);
		}
		retPath += "\\" + path;
	} else {
		Log::e() << "Error: " << result << "\n";
		retPath = "";
	}
	CoTaskMemFree(documentsDir);

	return retPath;

#elif defined(__linux__)
#	ifdef __arm__
	return "/home/pi/Documents/koala/" + path;
#	else
	std::string docsPath = "../Documents/Koala";
	if (!fs::exists(docsPath)) {
		fs::create_directories(docsPath);
	}
	return docsPath + "/" + path;
#	endif
#else
	Log::e() << "docsPath not implemented";
	return "";
#endif
}

string getAppId() {
#ifdef __APPLE__
	return [[[NSBundle mainBundle] bundleIdentifier] UTF8String];
#else
	Log::e() << "getAppId() does not work on this platform yet. Implement it in utils.cpp!";
	return "";
#endif
}

string appSupportPath(string path) {
#ifdef __APPLE__
#	if !TARGET_OS_IOS
	NSURL *url = [[[NSFileManager defaultManager] URLsForDirectory:NSApplicationSupportDirectory inDomains:NSUserDomainMask] lastObject];
	string _path = [[url path] UTF8String];
	_path += "/" + getAppId();
	if (!fs::exists(_path)) {
		fs::create_directory(_path);
	}
	return _path + "/" + path;
#	else
	Log::e() << "appSupportPath() not available on iOS";
	return "";
#	endif
#else
	Log::e() << "appSupportPath() not available on non-apple platforms";
	return "";
#endif
}

string getOSVersion() {
#ifdef __APPLE__
#	if TARGET_OS_IOS
	return [[[UIDevice currentDevice] systemVersion] UTF8String];
#	else
	NSProcessInfo *pInfo = [NSProcessInfo processInfo];
	NSString *version = [pInfo operatingSystemVersionString];
	return [version UTF8String];
#	endif
#elif defined(__ANDROID__)
	return androidGetOSVersion();
#else
	return "not implemented";
#endif
}

string getPlatformName() {
#ifdef __APPLE__
#	if TARGET_OS_IOS
	return "iOS";
#	else
	return "macOS";
#	endif
#elif defined(__ANDROID__)
	return "Android";
#elif defined(_WIN32)
	return "Windows";
#elif defined(__RPI)
	return "Raspberry Pi";
#elif defined(__linux__)
	return "Linux";
#else
	return "Unknown";
#endif
}
uint64_t getStorageRemainingInBytes() {
#ifdef __APPLE__

	NSURL *fileURL = [[[NSFileManager defaultManager] URLsForDirectory:NSDocumentDirectory inDomains:NSUserDomainMask] lastObject];

	NSError *error = nil;

	if (@available(macOS 10.13, iOS 11, *)) {
		NSDictionary *results = [fileURL resourceValuesForKeys:@[ NSURLVolumeAvailableCapacityForImportantUsageKey ] error:&error];

		if (!results) {
			NSLog(@"Error retrieving resource keys: %@\n%@", [error localizedDescription], [error userInfo]);

			return 1000000000;
			//		abort();
		}

		NSNumber *n = results[NSURLVolumeAvailableCapacityForImportantUsageKey];

		return n.longLongValue;
	} else {
		// Fallback on earlier versions
		Log::e() << "There's somethign wrong in getStorageRemainingInBytes()";
		return 1024 * 1024 * 1024;
	}

#elif defined(__ANDROID__)
	return androidGetStorageRemainingInBytes();
//#elif defined(_WIN32)
#else
	// this might actually be pretty good for all platforms, but need to test
	try {
		auto si = fs::space(".");
		return si.available;
	} catch (fs::filesystem_error const &e) {
		Log::e() << "Error in getStorageRemainingInBytes()";
		return 1024 * 1024 * 1024;
	}
#endif
}

// on iOS this'll give the launched url
string getLaunchUrl() {
	return Globals::launchUrl;
}

void setLaunchUrl(string url) {
	Globals::launchUrl = url;
}
string execute(string cmd, int *outExitCode) {
#ifdef __APPLE__
	//	printf("Executing %s\n", cmd.c_str());
	cmd += " 2>&1";
	FILE *pipe = popen(cmd.c_str(), "r");
	if (!pipe) return "ERROR";
	char buffer[128];
	std::string result = "";
	while (!feof(pipe)) {
		if (fgets(buffer, 128, pipe) != NULL) result += buffer;
	}

	int exitCode = pclose(pipe);
	if (outExitCode != nullptr) {
		*outExitCode = WEXITSTATUS(exitCode);
	}
	//	printf("%s\n", result.c_str());
	return result;
#else
	return "Error - can't do this";
#endif
}
void initMZGL(App *app) {
	if (!app->isHeadless()) {
		app->g.initGraphics();
	}
	Globals::startTime = std::chrono::system_clock::now();
}

#if TARGET_OS_IOS
#	include <UIKit/UIKit.h>
//#	import <WebKit/WebKit.h>
#endif

void launchUrl(string url) {
#ifdef __APPLE__
#	if TARGET_OS_IOS
	NSString *urlStr = [NSString stringWithUTF8String:url.c_str()];
	[[UIApplication sharedApplication] openURL:[NSURL URLWithString:urlStr]];
#	else
	NSString *urlStr = [NSString stringWithUTF8String:url.c_str()];
	[[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString:urlStr]];
#	endif
#elif defined(__ANDROID__)
	androidLaunchUrl(url);
#else
	Log::e() << "Launch url not implemented";
#endif
}

string getAppVersionString() {

	std::string version = "";
#ifdef __APPLE__
	NSString *str = [[NSBundle mainBundle] objectForInfoDictionaryKey:@"CFBundleShortVersionString"];
	if (str == nil) {
		return "not available";
	}
	std::string v = string([str UTF8String]);
	
	NSString *ver = [[NSBundle mainBundle] objectForInfoDictionaryKey:@"CFBundleVersion"];
	if (ver != nil) {
		
		v += " (" + string([ver UTF8String]) + ")";
	}

	
	version = v;
#elif defined(__ANDROID__)
	version = androidGetAppVersionString();
#else
	version = "No version available";

#endif
	
#ifdef DEBUG
	version += " DEBUG";
#endif
	return version;
}

#if defined(__APPLE__) && !TARGET_OS_IOS
// ofSystemUtils.cpp is configured to build as
// objective-c++ so as able to use Cocoa dialog panels
// This is done with this compiler flag
//		-x objective-c++
// http://www.yakyak.org/viewtopic.php?p=1475838&sid=1e9dcb5c9fd652a6695ac00c5e957822#p1475838

#	include <Cocoa/Cocoa.h>

#endif

void saveFileDialog(string msg, string defaultFileName, function<void(string, bool)> completionCallback) {
#ifdef AUTO_TEST
	completionCallback(defaultFileName, randi(2) == 0);
	return;
#endif
#ifdef __APPLE__
#	if !TARGET_OS_IOS
	dispatch_async(dispatch_get_main_queue(), ^{
	  // do work here
	  NSInteger buttonClicked;
	  string filePath = "";
	  @autoreleasepool {
		  NSSavePanel *saveDialog = [NSSavePanel savePanel];
		  NSOpenGLContext *context = [NSOpenGLContext currentContext];
		  [saveDialog setMessage:[NSString stringWithUTF8String:msg.c_str()]];
		  [saveDialog setNameFieldStringValue:[NSString stringWithUTF8String:defaultFileName.c_str()]];

		  buttonClicked = [saveDialog runModal];

		  [context makeCurrentContext];

		  if ([[[saveDialog URL] path] compare:[[saveDialog directoryURL] path]] == NSOrderedSame) {
			  completionCallback("", false);
			  Log::e() << "No filename given, abort! abort!";
			  return;
		  }
		  if (buttonClicked == NSModalResponseOK) {
			  filePath = string([[[saveDialog URL] path] UTF8String]);
		  }
	  }
	  completionCallback(filePath, buttonClicked == NSModalResponseOK);
	});
#	endif
#elif defined(_WIN32)

	wchar_t fileName[MAX_PATH] = L"";
	char *extension;
	OPENFILENAMEW ofn;
	memset(&ofn, 0, sizeof(OPENFILENAME));
	ofn.lStructSize = sizeof(OPENFILENAME);
	HWND hwnd = WindowFromDC(wglGetCurrentDC());
	ofn.hwndOwner = hwnd;
	ofn.hInstance = GetModuleHandle(0);
	ofn.nMaxFileTitle = 31;
	ofn.lpstrFile = fileName;
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrFilter = L"All Files (*.*)\0*.*\0";
	ofn.lpstrDefExt = L""; // we could do .rxml here?
	ofn.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY;
	ofn.lpstrTitle = L"Select Output File";

	if (GetSaveFileNameW(&ofn)) {
		std::wstring ws(fileName);
		std::string fileNameUtf8 = std::wstring_convert<std::codecvt_utf8<wchar_t>>().to_bytes(ws);
		completionCallback(fileNameUtf8, true);
	} else {
		completionCallback("", false);
	}
#elif !defined(__ANDROID__) && defined(__linux__)
	linuxSaveFileDialog(msg, defaultFileName, completionCallback);
#endif
}

void setCursor(Cursor cursor) {
#if defined(__APPLE__) && !TARGET_OS_IOS
	switch (cursor) {
		case Cursor::ARROW: [[NSCursor arrowCursor] set]; break;
		case Cursor::IBEAM: [[NSCursor IBeamCursor] set]; break;
		case Cursor::CROSSHAIR: [[NSCursor crosshairCursor] set]; break;
		case Cursor::OPEN_HAND: [[NSCursor openHandCursor] set]; break;
		case Cursor::CLOSED_HAND: [[NSCursor closedHandCursor] set]; break;
		case Cursor::POINTING_HAND: [[NSCursor pointingHandCursor] set]; break;
		case Cursor::LEFT_RESIZE: [[NSCursor resizeLeftCursor] set]; break;
		case Cursor::RIGHT_RESIZE: [[NSCursor resizeRightCursor] set]; break;
		case Cursor::UP_RESIZE: [[NSCursor resizeUpCursor] set]; break;
		case Cursor::DOWN_RESIZE: [[NSCursor resizeDownCursor] set]; break;
		case Cursor::LEFT_RIGHT_RESIZE: [[NSCursor resizeLeftRightCursor] set]; break;
		case Cursor::UP_DOWN_RESIZE: [[NSCursor resizeUpDownCursor] set]; break;
	}
#endif
}

void showMouse() {
#ifdef __APPLE__
#	if !TARGET_OS_IOS
	[NSCursor unhide];
#	endif
#endif
}

void hideMouse() {
#ifdef __APPLE__
#	if !TARGET_OS_IOS
	[NSCursor hide];
#	endif
#endif
}

string tolower(std::string s) {
	std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
	return s;
}

bool readFile(string filename, std::vector<unsigned char> &outData) {
	fs::ifstream strm(fs::u8path(filename), std::ios_base::binary);
	if (!strm) {
		printf("cannot open file\n");
		return false;
	}

	strm.unsetf(std::ios_base::skipws);
	std::istream_iterator<unsigned char> isi(strm), isiEOF;
	outData.assign(isi, isiEOF);
	if (!strm.eof()) {
		printf("read error\n");
		return false;
	}
	return true;
}

bool writeFile(const std::string &path, const std::vector<unsigned char> &data) {
	fs::ofstream outfile(fs::u8path(path), ios::out | ios::binary);
	if (outfile.fail()) return false;
	outfile.write((const char *) data.data(), data.size());
	if (outfile.fail()) return false;
	outfile.close();
	if (outfile.fail()) return false;
	return true;
}

std::vector<unsigned char> readFile(string filename) {
	std::vector<unsigned char> outData;
	readFile(filename, outData);
	return outData;
}

bool writeStringToFile(const std::string &path, const std::string &data) {
	fs::ofstream outfile(fs::u8path(path), ios::out);
	if (outfile.fail()) {
		Log::e() << "writeStringToFile() open failed: " << strerror(errno) << path;
		return false;
	}
	outfile << data;
	if (outfile.fail()) {
		Log::e() << "writeStringToFile() data write failed: " << strerror(errno);
		return false;
	}
	outfile.close();
	if (outfile.fail()) {
		Log::e() << "writeStringToFile() close failed: " << strerror(errno);
		return false;
	}
	return true;
}
bool readStringFromFile(const std::string &path, std::string &outStr) {
	fs::ifstream t(fs::u8path(path));
	if (t.fail()) return false;

	t.seekg(0, std::ios::end);
	if (t.fail()) return false;
	outStr.reserve(t.tellg());
	if (t.fail()) return false;
	t.seekg(0, std::ios::beg);
	if (t.fail()) return false;
	outStr.assign((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
	if (t.fail()) return false;
	return true;
}
#ifdef __APPLE__
os_log_t logObject = nullptr;
void oslog(string s) {
	if (logObject == nullptr) {
		logObject = os_log_create("com.elf-audio.koala", "testing log");
	}
	os_log_error(logObject, "%s", s.c_str());
}
#endif

#ifndef __APPLE__
#	include <random>
#	include <sstream>
/*
 * From here: https://stackoverflow.com/questions/24365331/how-can-i-generate-uuid-in-c-without-using-boost-library/58467162
 * aparently not unique enough, but for now it will do
 */
namespace uuid {
static std::random_device rd;
static std::mt19937 gen(rd());
static std::uniform_int_distribution<> dis(0, 15);
static std::uniform_int_distribution<> dis2(8, 11);

std::string generate_uuid_v4() {
	std::stringstream ss;
	int i;
	ss << std::hex;
	for (i = 0; i < 8; i++) {
		ss << dis(gen);
	}
	ss << "-";
	for (i = 0; i < 4; i++) {
		ss << dis(gen);
	}
	ss << "-4";
	for (i = 0; i < 3; i++) {
		ss << dis(gen);
	}
	ss << "-";
	ss << dis2(gen);
	for (i = 0; i < 3; i++) {
		ss << dis(gen);
	}
	ss << "-";
	for (i = 0; i < 12; i++) {
		ss << dis(gen);
	};
	return ss.str();
}
} // namespace uuid
#endif

std::string generateUUID() {
#ifdef __APPLE__

	NSUUID *uuid = [[NSUUID alloc] init];
	return std::string([uuid.UUIDString UTF8String]);
#else
	return uuid::generate_uuid_v4();
#endif
}

bool isTabletDevice() {
#if defined(__APPLE__) && TARGET_OS_IOS
	return UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPad;
#elif defined(__ANDROID__)
	// TODO: don't know how to detect if its a tablet on android
	// but maybe something to do with screen resolution and aspect ratio
	return false;
#else
	// assume its a desktop, so lets call it tablet
	return true;
#endif
}
