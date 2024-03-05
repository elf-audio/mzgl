#pragma once

#include <string>
#include <vector>
#include <functional>
#include <android/log.h>
#include "App.h"
#include "Midi.h"
#include "mzAssert.h"
#include "FloatBuffer.h"


#define LOGE(...) ((void) __android_log_print(ANDROID_LOG_ERROR, "native-activity", __VA_ARGS__))
#define LOGD(...) ((void) __android_log_print(ANDROID_LOG_DEBUG, "native-activity", __VA_ARGS__))
#define LOGI(...) ((void) __android_log_print(ANDROID_LOG_INFO, "native-activity", __VA_ARGS__))
#define LOGW(...) ((void) __android_log_print(ANDROID_LOG_WARN, "native-activity", __VA_ARGS__))

#include "android_native_app_glue.h"

// use this instead of fopen if the file is in assets, then you can use fread() and fclose()
// as normal - see http://www.50ply.com/blog/2013/01/19/loading-compressed-android-assets-with-file-pointer/#comment-1850768990
FILE *android_asset_open(const char *fname, const char *mode);

std::shared_ptr<App> androidGetApp();

std::vector<std::string> androidListAssetDir(const std::string &path);
bool loadAndroidAsset(const std::string &path, std::vector<unsigned char> &outData);
std::string loadAndroidAssetAsString(const std::string &path);

void listAndroidAssetDir(const std::string &path, std::vector<std::string> &outPaths);

std::string getAndroidInternalDataPath();
std::string getAndroidExternalDataPath();
std::string getAndroidExternalStorageDirectory();
void androidAlertDialog(const std::string &title, const std::string &msg);
bool androidHasMicPermission();
void androidConfirmDialog(std::string title,
						  std::string msg,
						  std::function<void()> okPressed,
						  std::function<void()> cancelPressed);

void androidTextboxDialog(std::string title,
						  std::string msg,
						  std::string text,
						  std::function<void(std::string, bool)> completionCallback);

void androidTwoOptionCancelDialog(std::string title,
								  std::string msg,
								  std::string buttonOneText,
								  std::function<void()> buttonOnePressed,
								  std::string buttonTwoText,
								  std::function<void()> buttonTwoPressed,
								  std::function<void()> cancelPressed);

void androidThreeOptionCancelDialog(std::string title,
									std::string msg,
									std::string buttonOneText,
									std::function<void()> buttonOnePressed,
									std::string buttonTwoText,
									std::function<void()> buttonTwoPressed,
									std::string buttonThreeText,
									std::function<void()> buttonThreePressed,
									std::function<void()> cancelPressed);

std::string androidGetAppVersionString();
void androidShareDialog(std::string message, std::string path, std::function<void(bool)> completionCallback);
void androidImageDialog(std::string copyToPath,
						std::function<void(bool success, std::string imgPath)> completionCallback);
void androidFileDialog(std::string copyToPath,
                        const std::vector<std::string> &allowedExtensions,
                        std::function<void(std::string resultingPath, bool success)> completionCallback);

void androidLaunchUrl(const std::string &url);

//------------------------------------------------------------------------------------------------------------------------------
///
/// @brief Display html document in WebView for Android
///
/// @param html String containing HTML document to display.
//------------------------------------------------------------------------------------------------------------------------------
void androidDisplayHtml(const std::string &html);

bool androidEncodeAAC(std::string pathToOutput, const FloatBuffer &buff, int numChannels, int sampleRate);

std::string getAndroidTempDir();
std::vector<std::string> androidGetMidiDeviceNames();

bool androidIsOnWifi();

bool isUsingHeadphones();
bool isUsingUSBInterface();
bool isUsingAirplay();
bool isUsingBluetoothHeadphones();
class AllMidiDevicesAndroidImpl;
void androidSetupAllMidiIns(std::shared_ptr<AllMidiDevicesAndroidImpl> impl);

android_app *getAndroidAppPtr();
class EventDispatcher;
EventDispatcher *getAndroidEventDispatcher();
void callJNI(std::string methodName,
			 std::string arg1,
			 std::string arg2,
			 std::string arg3,
			 std::string arg4,
			 std::string arg5);
void callJNI(std::string methodName, std::string arg1, std::string arg2, std::string arg3, std::string arg4);
void callJNI(std::string methodName, std::string arg1, std::string arg2, std::string arg3);
void callJNI(std::string methodName, std::string arg1, std::string arg2);
void callJNI(std::string methodName, std::string arg1);
void callJNI(std::string methodName);
void callJNI(std::string methodName, int32_t arg1);
void callJNI(std::string methodName, float arg1);

bool callJNIForBoolean(std::string methodName);
bool callJNIForBoolean(std::string methodName, int arg1);
bool callJNIForBoolean(std::string methodName, std::string arg1);
bool callJNIForBoolean(std::string methodName, const std::string &arg1, int arg2);
bool callJNIForBoolean(std::string methodName, const std::string &arg1, const std::string &arg2);

void callJNIForStringArray(std::string methodName, std::vector<std::string> &data);

float callJNIForFloat(std::string methodName);

int64_t callJNIForLong(std::string methodName);
int32_t callJNIForInt(std::string methodName);

std::string callJNIForString(std::string methodName, int32_t arg1);
int64_t callJNIForLong(std::string methodName, int32_t arg1);

void callJNI(std::string methodName, const std::string &arg1, int32_t arg2);

std::string callJNIForString(std::string methodName);
std::string callJNIForString(std::string methodName, std::string arg1);
std::string jstringToString(JNIEnv *jni, jstring text);
class ScopedJniAttachmentBlocker {
public:
	ScopedJniAttachmentBlocker() { shouldBlock = true; }
	~ScopedJniAttachmentBlocker() { shouldBlock = false; }
	static bool shouldBlock;
};
// these can be nested - if you have a nested scoped jni, only the outer one
// does anything, and the inner ones are ignored - handy if you need to make
// a block of Jni calls, so you don't keep having to Attach and DetatchCurrentThread
class ScopedJni {
public:
	ScopedJni() {
		//        Log::d() << "ScopedJni()";
		mzAssert(jni == nullptr);
		auto *appPtr = getAndroidAppPtr();
		if (appPtr != nullptr) {
			//            Log::d() << "attaching";
			//   int getEnvStat = appPtr->activity->vm->GetEnv((void**)&jni, JNI_VERSION_1_6);
			//     if(getEnvStat==JNI_EDETACHED) {
			// if(!ScopedJniAttachmentBlocker::shouldBlock)
			success = appPtr->activity->vm->AttachCurrentThread(&jni, nullptr) == JNI_OK;
			//  } else if (getEnvStat == JNI_EVERSION) {
			//      std::cout << "GetEnv: version not supported" << std::endl;
			//  }
		}
	}
	JNIEnv *j() { return jni; }
	jmethodID getMethodID(const std::string &methodName, const std::string &signature) {
		auto *cl = getClass();
		if (cl == nullptr) return nullptr;
		return jni->GetMethodID(cl, methodName.c_str(), signature.c_str());
	}

	jclass getClass() {
		if (!success) return nullptr;
		auto *appPtr = getAndroidAppPtr();
		if (appPtr != nullptr) {
			jclass clazz = jni->GetObjectClass(appPtr->activity->clazz);

			return clazz;
		}
		return nullptr;
	}
	~ScopedJni() {
		//        Log::d() << "~ScopedJni()";
		if (success) { //} && !ScopedJniAttachmentBlocker::shouldBlock) {
			//            Log::d() << "detaching";
			getAndroidAppPtr()->activity->vm->DetachCurrentThread();
			jni = nullptr;
		}
	}

private:
	bool success = false;
	JNIEnv *jni	 = nullptr;
};
