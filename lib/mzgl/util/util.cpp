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
#	include <ShObjIdl_core.h>
#	include <locale>
#	include <codecvt>
#	include <locale>
#	include <sstream>
#	include <processthreadsapi.h>
#endif

#include <chrono>
#include <fstream>

#include "log.h"
#include <iomanip>
#include <algorithm>

#include "filesystem.h"

#include "App.h"
#include "mainThread.h"
#include <thread>

// this from openframeworks
#ifndef _WIN32
#	include <pwd.h>
#	include <unistd.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <optional>
#include "filesystem.h"

void setThreadName(const std::string &name) {
#if defined(__APPLE__)
	pthread_setname_np(name.c_str());
#elif defined(_WIN32)
	const wchar_t *wName = reinterpret_cast<const wchar_t *>(name.data());
	HRESULT hr			 = SetThreadDescription(GetCurrentThread(), wName);
	if (FAILED(hr)) {
		Log::e() << "Error setting thread name: " << hr;
	}
#else
	pthread_setname_np(pthread_self(), name.c_str());
#endif
}

#ifndef _WIN32
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
std::string utf8decomposedToPrecomposed(const std::string &str) {
#ifdef __APPLE__
	// Convert std::string to NSString
	NSString *decomposedNSString = [NSString stringWithUTF8String:str.c_str()];

	// Normalize the NSString to precomposed form
	NSString *precomposedNSString = [decomposedNSString precomposedStringWithCanonicalMapping];

	return [precomposedNSString UTF8String];

#else
	// no implementation yet
	return str;
#endif
}

std::string utf8precomposedToDecomposed(const std::string &str) {
#ifdef __APPLE__
	// Convert std::string to NSString
	NSString *precomposedNSString = [NSString stringWithUTF8String:str.c_str()];

	// Normalize the NSString to decomposed form
	NSString *decomposedNSString = [precomposedNSString decomposedStringWithCanonicalMapping];

	return [decomposedNSString UTF8String];

#else
	// no implementation yet
	return str;
#endif
}

void deleteOrTrash(const std::string &path) {
	if (!fs::exists(fs::path {path})) {
		return;
	}

	auto stdDeleteFn = [path]() {
		try {
			fs::remove_all(path);
		} catch (const fs::filesystem_error &e) {
			Log::e() << "Error deleting file: " << e.what();
		}
	};
#ifdef __APPLE__
	if ([[NSFileManager defaultManager] respondsToSelector:@selector(trashItemAtURL:resultingItemURL:error:)]) {
		NSError *error = nil;
		BOOL success   = [[NSFileManager defaultManager]
				trashItemAtURL:[NSURL fileURLWithPath:[NSString stringWithUTF8String:path.c_str()]]
			  resultingItemURL:nil
						 error:&error];
		if (!success || error) {
			NSLog(@"Error moving file to trash: %@", error);
			stdDeleteFn();
		}
	} else {
		stdDeleteFn();
	}
#else
	stdDeleteFn();
#endif
}

bool copyDir(const fs::path &source, const fs::path &destination, std::string &errMsg) {
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
		errMsg = std::string(e.what());
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
			errMsg = std::string(e.what());
			return false;
		}
	}
	return true;
}

bool copyDir(const std::string &source, const std::string &destination, std::string &errMsg) {
	return copyDir(fs::path(source), fs::path(destination), errMsg);
}

void sleepMillis(long ms) {
	std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

void sleepMicros(long ms) {
	std::this_thread::sleep_for(std::chrono::microseconds(ms));
}

bool is_number(const std::string &s) {
	return !s.empty() && std::find_if(s.begin(), s.end(), [](char c) { return !std::isdigit(c); }) == s.end();
}

std::string uniquerizePath(const std::string &path) {
	fs::path p(path);
	std::string outPath = p.string();
	if (fs::exists(p)) {
		std::string filenameBase = p.stem().string();
		int lastSlash			 = (int) filenameBase.rfind("-");

		int startingIndex = 1;
		if (lastSlash != -1) {
			std::string ending = filenameBase.substr(lastSlash + 1);
			if (is_number(ending)) {
				filenameBase  = filenameBase.substr(0, lastSlash);
				startingIndex = stoi(ending) + 1;
			}
		}

		for (int i = startingIndex; i < 10000; i++) {
			auto filename = p.stem();
			auto pp		  = p.parent_path() / (filenameBase + "-" + std::to_string(i) + p.extension().string());

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

namespace Globals {
	std::string launchUrl = "";
	std::chrono::system_clock::time_point startTime;
	std::vector<std::string> commandLineArgs;
} // namespace Globals

void loadCommandLineArgs(int argc, const char *argv[]) {
	Globals::commandLineArgs.clear();
	for (int i = 0; i < argc; i++) {
		Globals::commandLineArgs.emplace_back(argv[i]);
	}
}

std::vector<std::string> getCommandLineArgs() {
	return Globals::commandLineArgs;
}

void addCommandLineFlag(const std::string &flag) {
	Globals::commandLineArgs.emplace_back(flag);
}

bool hasCommandLineFlag(const std::string &flag) {
#if MZGL_MAC || defined(DEBUG)
	auto args = getCommandLineArgs();
	return std::find_if(std::begin(args), std::end(args), [flag](auto &&arg) { return arg == flag; })
		   != std::end(args);
#endif
	return false;
}

std::string convertToSettingString(const std::string &setting) {
	return setting + "=";
}

bool hasCommandLineSetting(const std::string &setting) {
	auto args = getCommandLineArgs();
	return std::find_if(
			   std::begin(args),
			   std::end(args),
			   [setting](auto &&arg) { return arg.find(convertToSettingString(setting)) != std::string::npos; })
		   != std::end(args);
}

std::string getCommandLineSetting(const std::string &setting, const std::string &defaultValue) {
	auto args = getCommandLineArgs();
	auto iter = std::find_if(std::begin(args), std::end(args), [setting](auto &&arg) {
		return arg.find(convertToSettingString(setting)) != std::string::npos;
	});

	if (iter == std::end(args)) {
		return defaultValue;
	}

	auto arg = *iter;
	arg.erase(0, convertToSettingString(setting).length());
	return arg;
}

int getCommandLineSetting(const std::string &setting, int defaultValue) {
	return std::stoi(getCommandLineSetting(setting, std::to_string(defaultValue)));
}

float getSeconds() {
	auto end									  = std::chrono::system_clock::now();
	std::chrono::duration<double> elapsed_seconds = end - Globals::startTime;
	return elapsed_seconds.count();
}

#ifdef __APPLE__
#	if !TARGET_OS_IOS
#		include "MacAppDelegate.h"
#		include "EventsView.h"
#	endif
#endif
#include "EventDispatcher.h"
void setWindowSize(int w, int h) {
#ifdef __APPLE__
#	if TARGET_OS_IOS
	// do nothing. iPhones have fixed window size
#	else
	NSWindow *win  = [NSApp mainWindow];
	NSRect frame   = win.frame;
	NSSize newSize = CGSizeMake(w, h);

	frame.origin.y -= frame.size.height;
	frame.origin.y += newSize.height;
	frame.size = newSize;
	dispatch_async(dispatch_get_main_queue(), ^(void) {
	  [win setFrame:frame display:YES animate:NO];
	  EventsView *delegate = (EventsView *) win.delegate;
	  if (delegate == nullptr) {
		  return;
	  }
	  auto dispatcher			= [delegate getEventDispatcher];
	  dispatcher->app->g.width	= w;
	  dispatcher->app->g.height = h;
	  dispatcher->resized();

	  dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t) (USEC_PER_SEC)), dispatch_get_main_queue(), ^{
		MZGLView *view = (MZGLView *) win.delegate;
		[view windowResized:[[NSNotification alloc] initWithName:@"" object:win userInfo:nil]];
		[win display];
		[win center];
	  });
	});

#	endif
#endif
}

void setWindowTitle(const std::string &title) {
	std::string titleLocalCopy = title;
#if defined(__APPLE__) && !TARGET_OS_IOS

	dispatch_async(dispatch_get_main_queue(), ^(void) {
	  NSWindow *win = [NSApp mainWindow];
	  [win setTitle:[NSString stringWithUTF8String:titleLocalCopy.c_str()]];
	});
#endif
}

std::string tempDir() {
#if defined(__APPLE__) // && TARGET_OS_IOS
	std::string _p;
	if (@available(macOS 10.12, *)) {
		NSURL *url = [[NSFileManager defaultManager] temporaryDirectory];
		_p		   = [[url path] UTF8String];
	} else {
		// Fallback on earlier versions
		_p = [NSTemporaryDirectory() UTF8String];
	}

#elif defined(__ANDROID__)
	std::string _p = getAndroidTempDir();
#else
	std::string _p = fs::temp_directory_path().string();
#endif
	return _p;
}

bool isOverridingDataPath	 = false;
std::string dataPathOverride = "";

void setDataPath(const std::string &path) {
	isOverridingDataPath = true;
	dataPathOverride	 = path;
}

static fs::path getVSTBundlePath() {
#ifdef _WIN32
	auto path			  = getDLLPath();
	const auto parentPath = std::string {".."};
	return fs::canonical(path / parentPath / parentPath / parentPath);
#else
	// FIXME: This needs to be implemented for other platforms
	return "";
#endif
}

// TODO: we would have an option for mac to load from its bundle rather than the dataPath (i.e. mac and iOS use same code)
std::string dataPath(const std::string &path, const std::string &appBundleId) {
	// it's an absolute path, don't do anything to it
	if (!path.empty() && path[0] == '/') return path;

#ifdef UNIT_TEST
	if (isOverridingDataPath) return dataPathOverride + "/" + path;
#endif

#if defined(MZGL_PLUGIN_VST)
	const auto bundlePath {getVSTBundlePath()};
	const auto dataPath {bundlePath / "Contents" / "Resources" / "data"};
	return (dataPath / path).string();
#endif

#if defined(__APPLE__) && defined(MZGL_MAC_GLFW)

	return "data/" + path;
#elif defined(__APPLE__)

	if (appBundleId.empty()) {
		NSString *a	  = [[NSBundle mainBundle] resourcePath];
		std::string s = [a UTF8String];
		s += "/data/" + path;
		return s;
	} else {
		Log::e() << "Going for bundle" << appBundleId;
		NSBundle *pBundle = [NSBundle
			bundleWithIdentifier:[NSString stringWithCString:appBundleId.c_str() encoding:NSUTF8StringEncoding]];
		if (pBundle == nil) {
			return path;
		} else {
			std::string returnPath = std::string([[pBundle resourcePath] UTF8String]) + "/data/" + path;
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

//#ifdef UNIT_TEST
bool isOverridingDocsPath	 = false;
std::string docsPathOverride = "";

void setDocsPath(const std::string &path) {
	isOverridingDocsPath = true;
	docsPathOverride	 = path;
}
//#endif

#ifdef __APPLE__
#	include <os/proc.h>
#	include <mach/mach_host.h>
#	if !TARGET_OS_IOS
#		include <sys/sysctl.h>
#	endif
#endif

int64_t getAvailableMemory() {
#ifdef __APPLE__
#	if TARGET_OS_IOS
	if (@available(iOS 13.0, *)) {
		return os_proc_available_memory();
	}
#	else
	int request[] = {CTL_HW, HW_MEMSIZE};
	unsigned long long memory;
	auto memoryLength = sizeof(memory);

	if (sysctl(request, 2, &memory, &memoryLength, nullptr, 0) == 0) {
		return static_cast<int64_t>(memory);
	} else {
		Log::e() << "Failed to query available memory";
	}
	return -1;
#	endif
#endif
#ifdef __ANDROID__
	return androidGetAvailableMemory();
#endif
	static bool alreadyWarnedAboutGetAvailableMemory = false;
	if (!alreadyWarnedAboutGetAvailableMemory) {
		alreadyWarnedAboutGetAvailableMemory = true;
		Log::e() << "Warning - getAvailableMemory() doesn't work on this OS";
	}
	return -1;
}

std::string nowAsString() {
	auto now				= std::chrono::system_clock::now();
	std::time_t currentTime = std::chrono::system_clock::to_time_t(now);
	std::tm *timeInfo		= std::localtime(&currentTime);
	std::ostringstream oss;
	oss << std::put_time(timeInfo, "%Y-%m-%d--%H.%M.%S");
	return oss.str();
}

fs::path findNewestSubdirectory(const fs::path &parentPath) {
	fs::path newestSubdirectory;
	std::optional<fs::file_time_type> newestTime;

	for (const auto &entry: fs::directory_iterator(parentPath)) {
		if (entry.is_directory()) {
			auto lastModified = fs::last_write_time(entry);
			if (!newestTime.has_value() || lastModified > *newestTime) {
				newestTime		   = lastModified;
				newestSubdirectory = entry.path();
			}
		}
	}

	return newestSubdirectory;
}

void updateDocumentsDirectory() {
	fs::path testDir	  = fs::temp_directory_path() / "koala" / "unit-tests";
	fs::path documentsDir = testDir;

	if (hasCommandLineFlag("--reset-documents-directory")) {
		documentsDir = testDir / nowAsString();
		if (fs::exists(documentsDir)) {
			fs::remove_all(documentsDir);
		}
		fs::create_directories(documentsDir);
	} else if (hasCommandLineFlag("--use-last-documents-directory")) {
		documentsDir = findNewestSubdirectory(testDir);
	}

	Log::d() << "[TESTS]: Documents path: " << documentsDir.string().c_str();
	setDocsPath(documentsDir.string());

	if (!fs::exists(documentsDir / "settings")) {
		Log::d() << "[TESTS]: Documents path, making settings:  " << (documentsDir / "settings").string().c_str();
		fs::create_directories(documentsDir / "settings");
	}
}

void checkForDocumentsDirectoryUpdate() {
	static bool hasUpdated = false;
	if (hasUpdated) {
		return;
	}

	hasUpdated = true;

	if (!hasCommandLineFlag("--use-test-documents-directory")) {
		return;
	}

	updateDocumentsDirectory();
}

bool hasPrintedTheError = false;
std::string docsPath(const std::string &path) {
	checkForDocumentsDirectoryUpdate();
	if (isOverridingDocsPath) {
		return docsPathOverride + "/" + path;
	}

#ifdef __APPLE__
	NSURL *url = [[[NSFileManager defaultManager] URLsForDirectory:NSDocumentDirectory inDomains:NSUserDomainMask]
		lastObject];
	std::string _path = [[url path] UTF8String];
	if (_path == "/Users/marek/Documents") {
		_path = "/Users/marek/Library/Containers/com.elf-audio.koala-mac/Data/Documents";
		if (!hasPrintedTheError) {
			printf("=======================================================================\n\n"
				   "Hack to get this working on marek's computer with vscode because vscode "
				   "running the app in a way that prevents NSFileManager from working properly\n\n"
				   "=======================================================================\n\n");
			hasPrintedTheError = true;
		}
	}
	return _path + "/" + path;
#elif defined(__ANDROID__)
	return getAndroidExternalDataPath() + "/" + path;
#elif defined(_WIN32)

	wchar_t *documentsDir;
	HRESULT result =
		SHGetKnownFolderPath(FOLDERID_AppDataDocuments, KF_FLAG_CREATE | KF_FLAG_INIT, NULL, &documentsDir);
	std::string retPath;

	if (result == S_OK) {
		std::wstring ws(documentsDir);
		std::string documentsDirUtf8 = std::wstring_convert<std::codecvt_utf8<wchar_t>>().to_bytes(ws);

		TCHAR szExeFileName[MAX_PATH];
		GetModuleFileName(NULL, szExeFileName, MAX_PATH);
		retPath = documentsDirUtf8 + "\\Koala";
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
	fs::create_directories(docsPath);

	return docsPath + "/" + path;
#	endif
#else
	Log::e() << "docsPath not implemented";
	return "";
#endif
}

std::string getAppId() {
#ifdef __APPLE__
	return [[[NSBundle mainBundle] bundleIdentifier] UTF8String];
#else
	Log::e() << "getAppId() does not work on this platform yet. Implement it in utils.cpp!";
	return "";
#endif
}

std::string appSupportPath(const std::string &path) {
#ifdef __APPLE__
#	if !TARGET_OS_IOS
	NSURL *url		  = [[[NSFileManager defaultManager] URLsForDirectory:NSApplicationSupportDirectory
															inDomains:NSUserDomainMask] lastObject];
	std::string _path = [[url path] UTF8String];
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

std::string getOSVersion() {
#ifdef __APPLE__
#	if TARGET_OS_IOS
	return [[[UIDevice currentDevice] systemVersion] UTF8String];
#	else
	NSProcessInfo *pInfo = [NSProcessInfo processInfo];
	NSString *version	 = [pInfo operatingSystemVersionString];
	return [version UTF8String];
#	endif
#elif defined(__ANDROID__)
	return androidGetOSVersion();
#else
	return "not implemented";
#endif
}

std::string getPlatformName() {
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

	NSURL *fileURL = [[[NSFileManager defaultManager] URLsForDirectory:NSDocumentDirectory
															 inDomains:NSUserDomainMask] lastObject];

	NSError *error = nil;

	if (@available(macOS 10.13, iOS 11, *)) {
		// on mac, the NSURLVolumeAvailableCapacityForImportantUsageKey is more accurate
		// but it spits out loads of garbage to the console, so for now just using
		// the less accurate version so I can read logs.
#	if TARGET_OS_IOS
		auto flag = NSURLVolumeAvailableCapacityForImportantUsageKey;
#	else
		auto flag = NSURLVolumeAvailableCapacityKey;
#	endif
		NSDictionary *results = [fileURL resourceValuesForKeys:@[ flag ] error:&error];

		if (!results) {
			NSLog(@"Error retrieving resource keys: %@\n%@", [error localizedDescription], [error userInfo]);

			return 1000000000;
			//		abort();
		}

		NSNumber *n = results[flag];

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
		Log::e() << "Error in getStorageRemainingInBytes() - " << e.what();
		return 1024 * 1024 * 1024;
	}
#endif
}

// on iOS/mac this'll give the launched url
std::string getLaunchUrl() {
	return Globals::launchUrl;
}

void setLaunchUrl(const std::string &url) {
	Globals::launchUrl = url;
}
std::string execute(std::string cmd, int *outExitCode) {
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
void initMZGL(std::shared_ptr<App> app) {
	if (!app->isHeadless()) {
		app->g.initGraphics();
	}
	Globals::startTime	 = std::chrono::system_clock::now();
	app->g.frameDelta	 = 1.f / 60.f;
	app->g.currFrameTime = 0.f;
}

#if TARGET_OS_IOS
#	include <UIKit/UIKit.h>
//#	import <WebKit/WebKit.h>
#endif

void launchUrl(const std::string &url) {
#if MZGL_IOS

	NSURL *nsUrl = [NSURL URLWithString:[NSString stringWithUTF8String:url.c_str()]];

	if ([[UIApplication sharedApplication] canOpenURL:nsUrl]) {
		[[UIApplication sharedApplication] openURL:nsUrl options:@{} completionHandler:nil];
	}
#elif MZGL_MAC
	NSString *urlStr = [NSString stringWithUTF8String:url.c_str()];
	[[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString:urlStr]];
#elif MZGL_ANDROID
	androidLaunchUrl(url);
#else
	Log::e() << "Launch url not implemented";
#endif
}

std::string getAppVersionString() {
	std::string version;
#ifdef __APPLE__
	NSString *str = [[NSBundle mainBundle] objectForInfoDictionaryKey:@"CFBundleShortVersionString"];
	if (str == nil) {
		return "not available";
	}
	std::string v = std::string([str UTF8String]);

	NSString *ver = [[NSBundle mainBundle] objectForInfoDictionaryKey:@"CFBundleVersion"];
	if (ver != nil) {
		v += " (" + std::string([ver UTF8String]) + ")";
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
#ifdef EXPERIMENTAL
	version += " EXPERIMENTAL";
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
#	include <UniformTypeIdentifiers/UniformTypeIdentifiers.h>
#endif

void saveFileDialog(const std::string &msg,
					const std::string &defaultFileName,
					const std::vector<std::string> &allowedExtensions,
					std::function<void(std::string, bool)> completionCallback) {
#ifdef AUTO_TEST
	completionCallback(defaultFileName, randi(2) == 0);
	return;
#endif
#ifdef __APPLE__
#	if !TARGET_OS_IOS
	const auto msgCopy				 = msg;
	const auto defaultFileNameCopy	 = defaultFileName;
	const auto allowedExtensionsCopy = allowedExtensions;
	dispatch_async(dispatch_get_main_queue(), ^{
	  // do work here
	  NSModalResponse buttonClicked = -1;
	  std::string filePath;
	  @autoreleasepool {
		  NSSavePanel *saveDialog  = [NSSavePanel savePanel];
		  NSOpenGLContext *context = [NSOpenGLContext currentContext];
		  [saveDialog setMessage:[NSString stringWithUTF8String:msgCopy.c_str()]];
		  [saveDialog setNameFieldStringValue:[NSString stringWithUTF8String:defaultFileNameCopy.c_str()]];

		  if (@available(macOS 11.0, *)) {
			  NSMutableArray<UTType *> *exts = [[NSMutableArray alloc] init];
			  for (const auto &ext: allowedExtensionsCopy) {
				  NSString *nsExt = [NSString stringWithUTF8String:ext.substr(1).c_str()];

				  if (UTType *type = [UTType typeWithFilenameExtension:nsExt]) {
					  [exts addObject:type];
				  }
			  }
			  [saveDialog setAllowedContentTypes:exts];
		  }

		  buttonClicked = [saveDialog runModal];

		  [context makeCurrentContext];

		  if ([[[saveDialog URL] path] compare:[[saveDialog directoryURL] path]] == NSOrderedSame) {
			  completionCallback("", false);
			  Log::e() << "No filename given, abort! abort!";
			  return;
		  }
		  if (buttonClicked == NSModalResponseOK) {
			  filePath = std::string([[[saveDialog URL] path] UTF8String]);
		  }
	  }
	  completionCallback(filePath, buttonClicked == NSModalResponseOK);
	});
#	endif
#elif defined(_WIN32)
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	std::wstring wide = converter.from_bytes(defaultFileName);

	size_t extPos		  = defaultFileName.rfind('.');
	std::string extension = (extPos != std::string::npos) ? defaultFileName.substr(extPos + 1) : "";

	wchar_t fileName[MAX_PATH] = L"";
	wcsncpy(fileName, wide.c_str(), MAX_PATH - 1);
	fileName[MAX_PATH - 1] = 0; // null terminate, in case of long string

	OPENFILENAMEW ofn;
	memset(&ofn, 0, sizeof(OPENFILENAME));
	ofn.lStructSize	  = sizeof(OPENFILENAME);
	HWND hwnd		  = WindowFromDC(wglGetCurrentDC());
	ofn.hwndOwner	  = hwnd;
	ofn.hInstance	  = GetModuleHandle(0);
	ofn.nMaxFileTitle = 31;
	ofn.lpstrFile	  = fileName;
	ofn.nMaxFile	  = MAX_PATH;

	std::wstring filter		   = L"All Files (*.*)\0*.*\0";
	std::wstring wideExtension = L"";
	//	if (!extension.empty()) {
	//		wideExtension = converter.from_bytes(extension);
	//		filter		  = wideExtension + L" Files (*." + wideExtension + L")\0*." + wideExtension + L"\0" + filter;
	//	}

	for (const auto &extension: allowedExtensions) {
		if (!extension.empty()) {
			// Convert each extension to a wide string
			wideExtension = converter.from_bytes(extension);
			// Construct the filter part for this extension
			filter = wideExtension + L" Files (*" + wideExtension + L")\0*" + wideExtension + L"\0" + filter;
		}
	}

	ofn.lpstrFilter = filter.c_str(); // L"All Files (*.*)\0*.*\0";
	ofn.lpstrDefExt = wideExtension.c_str(); // Set the default extension
	ofn.Flags		= OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY;
	ofn.lpstrTitle	= L"Select Output File";

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

bool readFile(const std::string &filename, std::vector<unsigned char> &outData) {
	fs::ifstream strm(fs::u8path(filename), std::ios_base::binary);
	if (!strm) {
		printf("cannot open file with path '%s'\n", filename.c_str());
		return false;
	}

	strm.unsetf(std::ios_base::skipws);
	std::istream_iterator<unsigned char> isi(strm), isiEOF;
	outData.assign(isi, isiEOF);
	if (!strm.eof()) {
		printf("read error with file '%s'\n", filename.c_str());
		return false;
	}
	return true;
}

bool writeFile(const std::string &path, const std::vector<unsigned char> &data) {
	fs::ofstream outfile(fs::u8path(path), std::ios::out | std::ios::binary);
	if (outfile.fail()) return false;
	outfile.write(reinterpret_cast<const char *>(data.data()), data.size());
	if (outfile.fail()) return false;
	outfile.close();
	if (outfile.fail()) return false;
	return true;
}

std::vector<unsigned char> readFile(const std::string &filename) {
	std::vector<unsigned char> outData;
	readFile(filename, outData);
	return outData;
}

bool writeStringToFile(const std::string &path, const std::string &data) {
	fs::ofstream outfile(fs::u8path(path), std::ios::out);
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
	if (t.fail()) {
		Log::e() << "failed to open file at " << path;
		return false;
	}

	t.seekg(0, std::ios::end);
	if (t.fail()) {
		Log::e() << "failed to seek to end in " << path;
		return false;
	}
	outStr.reserve(t.tellg());
	if (t.fail()) {
		Log::e() << "failed to determine tellg " << path;
		return false;
	}
	t.seekg(0, std::ios::beg);
	if (t.fail()) {
		Log::e() << "failed to seek to beginning " << path;
		return false;
	}
	outStr.assign((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
	if (t.fail()) {
		Log::e() << "failed to read from " << path;
		return false;
	}
	return true;
}
bool moveFile(const std::string &from, const std::string &to) {
	try {
		if (!fs::copy_file(from, to)) {
			return false;
		}
		fs::remove(from);
		return true;
	} catch (const fs::filesystem_error &err) {
		Log::e() << "Exception thrown while attempting to move file - " << err.what();
		return false;
	}
}
#ifdef __APPLE__
os_log_t logObject = nullptr;
void oslog(const std::string &s) {
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
	return {[uuid.UUIDString UTF8String]};
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
