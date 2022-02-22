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

#include "mzgl_platform.h"
#ifdef __APPLE__
#include <os/log.h>
#include <TargetConditionals.h>
#include <Foundation/Foundation.h>
#if TARGET_OS_IOS
#	import <UIKit/UIKit.h>
#else
#	include <Cocoa/Cocoa.h>
#endif

#endif
#ifdef __ANDROID__
#include "androidUtil.h"
#include "koalaAndroidUtil.h"
#elif defined(__linux__)
#include "linuxUtil.h"
#endif

#ifdef _WIN32
#include "winUtil.h"
#include <windows.h>
#include <winuser.h>
#include <commdlg.h>
#define _WIN32_DCOM

#include <shlobj.h>
#include <tchar.h>
#include <stdio.h>

#endif



#include "Graphics.h"
#include <chrono>
#include <fstream>

#include "log.h"
#include "concurrentqueue.h"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include "Graphics.h"

#include "filesystem.h"

#include "App.h"

using namespace std;
#include <thread>
static std::thread::id		mainThreadId;



// this from openframeworks
#ifdef _WIN32
#include <locale>
#include <sstream>
#include <string>

std::string convertWideToNarrow( const wchar_t *s, char dfault = '?',
                      const std::locale& loc = std::locale() )
{
  std::ostringstream stm;

  while( *s != L'\0' ) {
    stm << std::use_facet< std::ctype<wchar_t> >( loc ).narrow( *s++, dfault );
  }
  return stm.str();
}
#endif

#include <stdlib.h>
#include <stdio.h>

#ifndef _WIN32
#include <pwd.h>
#include <unistd.h>

std::string getHomeDirectory() {
    const char *homeDir = getenv("HOME");

    if (!homeDir) {
        struct passwd* pwd = getpwuid(getuid());
        if (pwd)
           homeDir = pwd->pw_dir;
    }
    return homeDir;
}
#endif




bool copyDir(const fs::path &source, const fs::path &destination, string &errMsg) {
			
	try {
		// Check whether the function call is valid
		if(!fs::exists(source) || !fs::is_directory(source)) {
			errMsg = "Source directory " + source.string() + " does not exist or is not a directory.";
			return false;
		}
		
		if(fs::exists(destination)) {
			errMsg = "Destination directory " + destination.string() + " already exists.";
			return false;
		}
		
		// Create the destination directory
		if(!fs::create_directory(destination)) {
			errMsg = "Unable to create destination directory" + destination.string();
			return false;
		}
	} catch(fs::filesystem_error const & e) {
		errMsg = string(e.what());
		return false;
	}
	
	// Iterate through the source directory
	for(fs::directory_iterator file(source); file != fs::directory_iterator(); ++file) {
		try {
			fs::path current(file->path());
			if(fs::is_directory(current)) {
				// Found directory: Recursion
				if(!copyDir(current, destination / current.filename(), errMsg)) return false;

			} else {
				// Found file: Copy
				fs::copy_file(current, destination / current.filename());
			}
		} catch(fs::filesystem_error const & e) {
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


bool is_number(const std::string& s)
{
	return !s.empty() && std::find_if(s.begin(),
									  s.end(), [](char c) { return !std::isdigit(c); }) == s.end();
}
template <typename T>
std::string to_string(const T a_value, const int n)
{
	std::ostringstream out;
	out << std::setprecision(n) << a_value;
	return out.str();
}

string uniquerizePath(string path) {
	fs::path p(path);
	string outPath = p.string();
	if(fs::exists(p)) {
		string filenameBase = p.stem().string();
		int lastSlash = (int)filenameBase.rfind("-");
		
		int startingIndex = 1;
		if(lastSlash!=-1) {
			string ending = filenameBase.substr(lastSlash+1);
			if(is_number(ending)) {
				filenameBase = filenameBase.substr(0, lastSlash);
				startingIndex = stoi(ending) + 1;
			}
		}
		
		for(int i = startingIndex; i < 10000; i++) {
			auto filename = p.stem();
			auto pp = p.parent_path() / (filenameBase + "-" + to_string(i) + p.extension().string());
			
			if(!fs::exists(pp)) {
				outPath = pp.string();
				break;
			}
		}
		if(fs::exists(outPath)) {
			Log::e() << "ERROR: can't make unique file name, ran out of numbers!!";
		}
	}
	return outPath;
}


vector<string> commandLineArgs;
void loadCommandLineArgs(int argc, const char *argv[]) {
	commandLineArgs.clear();
	for(int i = 0; i < argc; i++) {
		commandLineArgs.push_back(argv[i]);
	}
}

std::vector<std::string> getCommandLineArgs() {
	return commandLineArgs;
}

namespace Globals {
	float seconds = 0;
	string launchUrl = "";
	unsigned int frameNum = 0;
	std::chrono::system_clock::time_point startTime;
}

float getSeconds() {
	auto end = std::chrono::system_clock::now();
	std::chrono::duration<double> elapsed_seconds = end-Globals::startTime;
	return elapsed_seconds.count();
}

unsigned int getFrameNum() {
	return Globals::frameNum;
}

moodycamel::ConcurrentQueue<function<void()>> mainThreadQueue;

void runOnMainThread(function<void()> fn) {
    mzAssert(!isMainThread());
	mainThreadQueue.enqueue(fn);
}


void runOnMainThreadAndWait(function<void()> fn) {
	if(isMainThread()) {
		Log::e() << "runOnMainThreadAndWait() called from main thread";
		fn();
		return;
	}
	mzAssert(!isMainThread());
	atomic<bool> done {false};
	mainThreadQueue.enqueue([fn,&done]() {
		fn();
		done.store(true);
	});
	
	while(!done.load()) {
		std::this_thread::sleep_for(std::chrono::microseconds(100));
	}
}


void runOnMainThread(bool checkIfOnMainThread, function<void()> fn) {
	if(checkIfOnMainThread && isMainThread()) {
		fn();
	} else {
		runOnMainThread(fn);
	}
}
#ifdef UNIT_TEST
// dangerous to use in anything but testing
void clearMainThreadQueue() {
	mzAssert(isMainThread());
	function<void()> fn;
	while(mainThreadQueue.try_dequeue(fn)) {}
}
#endif

void pollMainThreadQueue() {
	function<void()> fn;
	while(mainThreadQueue.try_dequeue(fn)) {
		fn();
	}
}

void updateInternal() {
	++Globals::frameNum;
	pollMainThreadQueue();
}




#ifdef __APPLE__
#if !TARGET_OS_IOS
#include "MacAppDelegate.h"
#endif
#endif

void setWindowSize(int w, int h) {
#ifdef __APPLE__
#if TARGET_OS_IOS
	// do nothing. iPhones have fixed window size
#else
	NSWindow *win = [NSApp mainWindow];
//	win.frame.size = CGSizeMake(2000, 2000);
	
	// these lines were commented out because we don't have SCALE_FACTOR any more.
	// it's now g.pixelScale
//	w /= SCALE_FACTOR;
//	h /= SCALE_FACTOR;
	
	NSRect frame = win.frame;
	NSSize newSize = CGSizeMake(w, h);
	
	frame.origin.y -= frame.size.height;
	frame.origin.y += newSize.height;
	frame.size = newSize;
	dispatch_async(dispatch_get_main_queue(), ^(void){
		[win setFrame: frame display: YES animate: NO];
	});
	

#endif
#endif

}


void setWindowTitle(string title) {

#if defined(__APPLE__) && !TARGET_OS_IOS
	
	dispatch_async(dispatch_get_main_queue(), ^(void){
		NSWindow *win = [NSApp mainWindow];
		[win setTitle: [NSString stringWithUTF8String:title.c_str()]];
	});
#endif
}

string tempDir() {
#if defined(__APPLE__)// && TARGET_OS_IOS
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
	if(path.size()>0 && path[0]=='/') return path;

#ifdef UNIT_TEST
	if(isOverridingDataPath) return dataPathOverride + "/" + path;
#endif

#if defined(__APPLE__) && defined(MZGL_MAC_GLFW)
	
	return "data/" + path;
#elif defined(__APPLE__)
	
	if(appBundleId=="") {
//		Log::e() << "Going for main bundle";
		NSString *a = [[NSBundle mainBundle] resourcePath];
		string s = [a UTF8String];
		s += "/data/" + path;
//		Log::e() << s;
		return s;
	} else {
		Log::e() << "Going for bundle" << appBundleId;
		NSBundle* pBundle = [NSBundle bundleWithIdentifier:[NSString stringWithCString:appBundleId.c_str() encoding:NSUTF8StringEncoding]];
		if(pBundle==nil) {
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

string docsPath(string path) {
#ifdef UNIT_TEST
	if(isOverridingDocsPath) {
		return docsPathOverride + "/" + path;
	}
#endif
#ifdef __APPLE__
	NSURL *url =  [[[NSFileManager defaultManager] URLsForDirectory:NSDocumentDirectory inDomains:NSUserDomainMask] lastObject];
	string _path = [[url path] UTF8String];
		
	return _path + "/" + path;
#elif defined(__ANDROID__)
	return getAndroidExternalDataPath() + "/" + path;
#elif defined(_WIN32)
    CHAR my_documents[MAX_PATH];
    HRESULT result = SHGetFolderPath(NULL, CSIDL_PERSONAL, NULL, SHGFP_TYPE_CURRENT, my_documents);

    TCHAR szExeFileName[MAX_PATH];
    GetModuleFileName(NULL, szExeFileName, MAX_PATH);
    string pth = string(my_documents) + "\\" + fs::path(string(szExeFileName)).stem().string();
    if (result != S_OK) {
        Log::e() << "Error: " << result << "\n";
        return "";
    } else {
        // create dir if not exist
        if(!fs::exists(pth)) {
            fs::create_directory(pth);
        }
        pth += "\\" + path;

        return pth;
    }
#elif defined(__linux__)
#   ifdef __arm__
	return "/home/pi/Documents/koala/" + path;
#   else
    std::string docsPath = "../Documents/Koala";
    if(!fs::exists(docsPath)) {
        fs::create_directories(docsPath);
    }
    return docsPath + "/" + path;
#   endif
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
	NSURL *url = [[[NSFileManager defaultManager] URLsForDirectory: NSApplicationSupportDirectory inDomains: NSUserDomainMask] lastObject];
	string _path = [[url path] UTF8String];
	_path += "/" + getAppId();
	if(!fs::
	   exists(_path)) {
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


string byteSizeToString(uint64_t bytes) {
	
	char buf[256];
	double size = bytes;
	int i = 0;
	const char* units[] = {"B", "kB", "MB", "GB", "TB", "PB", "EB", "ZB", "YB"};
	while (size > 1024) {
		size /= 1024;
		i++;
	}
	sprintf(buf, "%.*f %s", i, size, units[i]);
	return buf;
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

	
	NSURL *fileURL =  [[[NSFileManager defaultManager] URLsForDirectory:NSDocumentDirectory inDomains:NSUserDomainMask] lastObject];
	
	NSError *error = nil;
	
	if (@available(macOS 10.13, iOS 11, *)) {
		
		NSDictionary *results = [fileURL resourceValuesForKeys:@[NSURLVolumeAvailableCapacityForImportantUsageKey] error:&error];
		
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
    } catch(fs::filesystem_error const & e) {
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
	FILE* pipe = popen(cmd.c_str(), "r");
	if (!pipe) return "ERROR";
	char buffer[128];
	std::string result = "";
	while(!feof(pipe)) {
		if(fgets(buffer, 128, pipe) != NULL)
			result += buffer;
	}
	
	int exitCode = pclose(pipe);
	if(outExitCode!=nullptr) {
		*outExitCode = WEXITSTATUS(exitCode);
	}
//	printf("%s\n", result.c_str());
	return result;
#else
return "Error - can't do this";
#endif
}
void initMZGL(App *app) {
	if(!app->isHeadless()) {
		app->g.initGraphics();
	}
	Globals::startTime = std::chrono::system_clock::now();
	setMainThreadId();
}

void setMainThreadId() {
	mainThreadId = std::this_thread::get_id();
}
// TODO: DELETE THIS
#include "EventDispatcher.h"
/*
void drawFrame(Graphics &g, EventDispatcher *eventDispatcher) {
	
	if(g.firstFrame) {
		initMZGL(eventDispatcher->app->g);
		eventDispatcher->setup();
		g.firstFrame = false;
	}


	#ifdef DO_DRAW_STATS
	Vbo::resetDrawStats();
	#endif

	g.setupView();
	updateInternal();
	eventDispatcher->update();
	callUpdateListeners();
	eventDispatcher->draw();
	callDrawListeners();
}
*/




void replaceAll(std::string & d, std::string toSearch, std::string replaceStr) {
	
	
	// Get the first occurrence
	size_t pos = d.find(toSearch);
	
	// Repeat till end is reached
	while( pos != std::string::npos) {
		// Replace this occurrence of Sub String
		d.replace(pos, toSearch.size(), replaceStr);
		// Get the next occurrence from the current position
		pos = d.find(toSearch, pos + replaceStr.size());
	}
}

//--------------------------------------------------
vector <string> split(const string & source, const string & delimiter, bool ignoreEmpty, bool _trim) {
	vector<string> result;
	if (delimiter.empty()) {
		result.push_back(source);
		return result;
	}
	string::const_iterator substart = source.begin(), subend;
	while (true) {
		subend = search(substart, source.end(), delimiter.begin(), delimiter.end());
		string sub(substart, subend);
		if(_trim) {
			sub = trim(sub);
		}
		if (!ignoreEmpty || !sub.empty()) {
			result.push_back(sub);
		}
		if (subend == source.end()) {
			break;
		}
		substart = subend + delimiter.size();
	}
	return result;
}
//--------------------------------------------------
string trimFront(const string & src){
	auto dst = src;
	dst.erase(dst.begin(),std::find_if_not(dst.begin(),dst.end(),[&](char & c){return std::isspace(c);}));
	return dst;
}

//--------------------------------------------------
string trimBack(const string & src){
	auto dst = src;
	dst.erase(std::find_if_not(dst.rbegin(),dst.rend(),[&](char & c){return std::isspace(c);}).base(), dst.end());
	return dst;
}

//--------------------------------------------------
string trim(const string & src){
	return trimFront(trimBack(src));
}


#include <sstream>
#include <iomanip>

std::string to_string(float value, int precision){
	std::ostringstream out;
	out << std::fixed << std::setprecision(precision) << value;
	return out.str();
}
std::string to_string(double value, int precision){
	return to_string((float)value, precision);
}


#if TARGET_OS_IOS
#	include <UIKit/UIKit.h>
//#	import <WebKit/WebKit.h>
#endif

void launchUrl(string url) {
#ifdef __APPLE__
#	if TARGET_OS_IOS
		NSString *urlStr = [NSString stringWithUTF8String: url.c_str()];
		[[UIApplication sharedApplication] openURL:[NSURL URLWithString:urlStr]];
#	else
		NSString *urlStr = [NSString stringWithUTF8String: url.c_str()];
		[[NSWorkspace sharedWorkspace] openURL: [NSURL URLWithString: urlStr]];
#	endif
#elif defined(__ANDROID__)
	androidLaunchUrl(url);
#else
	Log::e() << "Launch url not implemented";
#endif

}



string getAppVersionString() {
#ifdef __APPLE__
    NSString *str = [[NSBundle mainBundle] objectForInfoDictionaryKey:@"CFBundleShortVersionString"];
	if(str==nil) {
		return "not available";
	}
    return string([str UTF8String]);
#elif defined(__ANDROID__)
	return androidGetAppVersionString();
#else
	return "No version available";

#endif
}



#if defined(__APPLE__) && !TARGET_OS_IOS
// ofSystemUtils.cpp is configured to build as
// objective-c++ so as able to use Cocoa dialog panels
// This is done with this compiler flag
//		-x objective-c++
// http://www.yakyak.org/viewtopic.php?p=1475838&sid=1e9dcb5c9fd652a6695ac00c5e957822#p1475838

#include <Cocoa/Cocoa.h>



#endif


void saveFileDialog(string msg, string defaultFileName, function<void(string, bool)> completionCallback) {
#ifdef AUTO_TEST
	completionCallback(defaultFileName, randi(2)==0);
	return;
#endif
#ifdef __APPLE__
#if !TARGET_OS_IOS
	dispatch_async(dispatch_get_main_queue(), ^{
		// do work here
		NSInteger buttonClicked;
		string filePath = "";
		@autoreleasepool {
			NSSavePanel * saveDialog = [NSSavePanel savePanel];
			NSOpenGLContext *context = [NSOpenGLContext currentContext];
			[saveDialog setMessage:[NSString stringWithUTF8String:msg.c_str()]];
			[saveDialog setNameFieldStringValue:[NSString stringWithUTF8String:defaultFileName.c_str()]];
			
			
			buttonClicked = [saveDialog runModal];
			
			
			
			[context makeCurrentContext];
			
            if([[[saveDialog URL] path] compare: [[saveDialog directoryURL] path]]==NSOrderedSame) {
                completionCallback("", false);
                Log::e() << "No filename given, abort! abort!";
                return;
            }
			if(buttonClicked == NSModalResponseOK){
				filePath = string([[[saveDialog URL] path] UTF8String]);
			}
		}
		completionCallback(filePath, buttonClicked == NSModalResponseOK);
		
	});
#endif
#elif defined (_WIN32)


    wchar_t fileName[MAX_PATH] = L"";
    char * extension;
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
    ofn.lpstrDefExt = L"";	// we could do .rxml here?
    ofn.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY;
    ofn.lpstrTitle = L"Select Output File";

    if (GetSaveFileNameW(&ofn)){
        completionCallback(convertWideToNarrow(fileName), true);
    } else {
        completionCallback("", false);
    }
#elif !defined(__ANDROID__) && defined(__linux__)
	linuxSaveFileDialog(msg, defaultFileName, completionCallback);
#endif

}

//
//#if defined(__APPLE__) && !TARGET_OS_IOS
//@interface MyFilePickerDelegate : NSObject <NSOpenSavePanelDelegate> {
//	
//}
//-(void) setAllowedExtensions:(NSArray *)extensions;
//@end
//
//@implementation MyFilePickerDelegate {
//	NSArray *allowedExts;
//	BOOL allowAll;
//}
//
//-(id) init {
//	self = [super init];
//	if(self != nil) {
//		allowAll = YES;
//		allowedExts = @[@"png", @"tiff", @"jpg", @"gif", @"jpeg"];
//	}
//	return self;
//}
//-(void) setAllowedExtensions: (NSArray *)exts {
//	allowedExts = exts;
//	allowAll = NO;
//}
//
//
//
//- (BOOL)panel:(id)sender shouldEnableURL:(NSURL *)url {
//	if(allowAll) return YES;
//	
//	NSString* ext = [url pathExtension];
//	if ([ext isEqualToString: @""] || [ext isEqualToString: @"/"] || ext == nil || ext == nil || [ext length] < 1) {
//		return YES;
//	}
//
//	for(NSString *e in allowedExts) {
//		if ([ext caseInsensitiveCompare:e] == NSOrderedSame) {
//			return YES;
//		}
//	}
//	return NO;
//}
//
//
//@end
//MyFilePickerDelegate *impik_D = nil;
//#endif



//void loadFileDialog(string msg, function<void(string, bool)> completionCallback) {
//	loadFileDialog(msg, {}, completionCallback);
//}
//
//void loadFileDialog(string msg, const vector<string> &allowedExtensions, function<void(string, bool)> completionCallback) {
//#ifdef AUTO_TEST
//	return;
//#endif
//#ifdef _WIN32
//
//    OPENFILENAME ofn;
//
//    ZeroMemory(&ofn, sizeof(ofn));
//    ofn.lStructSize = sizeof(ofn);
//    HWND hwnd = WindowFromDC(wglGetCurrentDC());
//    ofn.hwndOwner = hwnd;
//
//  //  wchar_t szFileName[MAX_PATH] = L"";
//    char szFileName[MAX_PATH] = "";
//    ofn.lpstrFilter = "All\0";
//    ofn.lpstrFile = szFileName;
//
//    ofn.nMaxFile = MAX_PATH;
//    ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
//    ofn.lpstrDefExt = 0;
//
//    if(GetOpenFileName(&ofn)) {
//        completionCallback(szFileName, true);
////completionCallback(convertWideToNarrow(szFileName), true);
//    } else {
//        completionCallback("", false);
//    }
//
//#elif defined(__APPLE__)
//
//#	if !TARGET_OS_IOS
//	auto allowedExts = allowedExtensions;
//	dispatch_async(dispatch_get_main_queue(), ^{
//		// do work here
//		NSInteger buttonClicked;
//		string filePath = "";
//		@autoreleasepool {
//			NSOpenPanel * loadDialog = [NSOpenPanel openPanel];
//
//
//			if(allowedExts.size()>0) {
//				if(impik_D==nil) {
//					impik_D = [[MyFilePickerDelegate alloc] init];
//				}
//				vector<NSString*> nsExts;
//				for(const auto &ext: allowedExts) {
//					nsExts.push_back([NSString stringWithUTF8String:ext.c_str()]);
//				}
//				NSArray *exts = [NSArray arrayWithObjects:&nsExts[0] count: nsExts.size()];
//				[impik_D setAllowedExtensions:exts];
//				loadDialog.delegate = impik_D;
//			}
//
//			NSOpenGLContext *context = [NSOpenGLContext currentContext];
//			[loadDialog setMessage:[NSString stringWithUTF8String:msg.c_str()]];
////			[Dialog setNameFieldStringValue:[NSString stringWithUTF8String:defaultFileName.c_str()]];
//
//			buttonClicked = [loadDialog runModal];
//
//			[context makeCurrentContext];
//
//			if(buttonClicked == NSModalResponseOK){
//				filePath = string([[[loadDialog URL] path] UTF8String]);
//			}
//		}
//		completionCallback(filePath, buttonClicked == NSModalResponseOK);
//
//	});
//#	endif
//#elif !defined(__ANDROID__) && defined(__linux__)
//	linuxLoadFileDialog(msg, allowedExtensions, completionCallback);
//#endif
//
//}


void setCursor(Cursor cursor) {
#if defined(__APPLE__) && !TARGET_OS_IOS
	switch(cursor) {
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
#if !TARGET_OS_IOS
	[NSCursor unhide];
#endif
#endif
}

void hideMouse() {
#ifdef __APPLE__
#if !TARGET_OS_IOS
	[NSCursor hide];
#endif
#endif
}

// from cinder
bool isMainThread() {
	return std::this_thread::get_id() == mainThreadId;
}


#include <list>
#include <future>

class Task {
public:

	// runs taskFn asynchronously
	static void run(std::function<void()> taskFn) {
		
		auto addTaskFn = [taskFn]() {
//			taskMutex.lock();
			tasks.remove_if([](const shared_ptr<Task> task) { return task->done.load();});
			tasks.emplace_back(make_shared<Task>(taskFn));
//			taskMutex.unlock();
		};
		
        runOnMainThread(true, addTaskFn);
	}
//	virtual ~Task() {
//		printf("~Task() - %x\n", (void*)this);
//	}

	static void waitTilAllTasksAreDone() {
		while(tasks.size()>0) {
//			taskMutex.lock();
			tasks.remove_if([](const shared_ptr<Task> task) { return task->done.load(); });
//			taskMutex.unlock();
			sleepMillis(1);
		}
	}
//private:
	
	std::future<void> taskFuture;
	atomic<bool> done { false };
	
	Task(std::function<void()> taskFn) {

		taskFuture = std::async(std::launch::async, [this, taskFn](){
#if defined(__APPLE__) && DEBUG
				pthread_setname_np("runTask()");
#endif
			try {
				taskFn();
			} catch(const std::exception& err) {
				std::string ex = err.what();
				runOnMainThread([ex]() {
					Log::e() << "exception in runTask: " << ex;
					//alertDialog("ERROR", "Got an error: '" + ex + "' - please make a screenshot and report to marek@elf-audio.com");
				});
//				Log::e() << "Unhandled exception on runTask thread '" << err.what() << "'";
////				runOnMainThread([&err]() {
////					Log::e() << "Unhandled exception on runTask thread '" << err.what() << "'";
////					throw err;
////				});
			}
			
            done.store(true);
			
			runOnMainThread([]() {
				tasks.remove_if([](const shared_ptr<Task> task) { return task->done.load();});
			});
		});
	}
//	static std::mutex taskMutex;
	static std::list<shared_ptr<Task>> tasks;
};

std::list<shared_ptr<Task>> Task::tasks;
//std::mutex Task::taskMutex;

#ifdef UNIT_TEST
void waitTilAllTasksAreDone() {
	Task::waitTilAllTasksAreDone();
}
#endif
void runTask(function<void()> fn) {
	Task::run(fn);
}


string tolower(std::string s) {
	std::transform(s.begin(), s.end(), s.begin(),
				   [](unsigned char c){ return std::tolower(c); }  );
	return s;
}

bool readFile(string filename, std::vector<unsigned char> &outData) {
	
	ifstream strm(filename, std::ios_base::binary);
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
	ofstream outfile(path, ios::out | ios::binary);
	if(outfile.fail()) return false;
	outfile.write((const char*)data.data(), data.size());
	if(outfile.fail()) return false;
	outfile.close();
	if(outfile.fail()) return false;
	return true;
}


std::vector<unsigned char> readFile(string filename) {
	std::vector<unsigned char> outData;
	readFile(filename, outData);
	return outData;
}


bool writeStringToFile(const std::string &path, const std::string &data) {
	ofstream outfile(path, ios::out);
	if(outfile.fail()) {
		Log::e() << "writeStringToFile() open failed: " << strerror(errno) << path;
		return false;
	}
	outfile << data;
	if(outfile.fail()) {
		Log::e() << "writeStringToFile() data write failed: " << strerror(errno);
		return false;
	}
	outfile.close();
	if(outfile.fail()) {
		Log::e() << "writeStringToFile() close failed: " << strerror(errno);
		return false;
	}
	return true;
}
bool readStringFromFile(const std::string &path, std::string &outStr) {
	std::ifstream t(path);
	if(t.fail()) return false;
	

	t.seekg(0, std::ios::end);
	if(t.fail()) return false;
	outStr.reserve(t.tellg());
	if(t.fail()) return false;
	t.seekg(0, std::ios::beg);
	if(t.fail()) return false;
	outStr.assign((std::istreambuf_iterator<char>(t)),
				std::istreambuf_iterator<char>());
	if(t.fail()) return false;
	return true;
}
#ifdef __APPLE__
os_log_t logObject = nullptr;
void oslog(string s) {
	if(logObject==nullptr) {
		logObject = os_log_create("com.elf-audio.koala", "testing log");
	}
	os_log_error(logObject, "%s", s.c_str());
}
#endif


bool assertsEnabled = true;

void mzEnableAssert(bool enabled) {
	assertsEnabled = enabled;
}

bool mzAssertEnabled() {
	return assertsEnabled;
}



#ifndef __APPLE__
#include <random>
#include <sstream>
/*
 * From here: https://stackoverflow.com/questions/24365331/how-can-i-generate-uuid-in-c-without-using-boost-library/58467162
 * aparently not unique enough, but for now it will do
 */
namespace uuid {
	static std::random_device              rd;
	static std::mt19937                    gen(rd());
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
}
#endif


std::string generateUUID() {
#ifdef __APPLE__

	NSUUID *uuid = [[NSUUID alloc] init];
	return std::string([uuid.UUIDString UTF8String]);
#else
	return uuid::generate_uuid_v4();
#endif
}
