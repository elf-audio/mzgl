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
void androidConfirmDialog(const std::string &title,
						  const std::string &msg,
						  std::function<void()> okPressed,
						  std::function<void()> cancelPressed);

void androidTextboxDialog(const std::string &title,
						  const std::string &msg,
						  const std::string &text,
						  std::function<void(std::string, bool)> completionCallback);

void androidTwoOptionCancelDialog(const std::string &title,
								  const std::string &msg,
								  const std::string &buttonOneText,
								  std::function<void()> buttonOnePressed,
								  const std::string &buttonTwoText,
								  std::function<void()> buttonTwoPressed,
								  std::function<void()> cancelPressed);

void androidThreeOptionCancelDialog(const std::string &title,
									const std::string &msg,
									const std::string &buttonOneText,
									std::function<void()> buttonOnePressed,
									const std::string &buttonTwoText,
									std::function<void()> buttonTwoPressed,
									const std::string &buttonThreeText,
									std::function<void()> buttonThreePressed,
									std::function<void()> cancelPressed);

std::string androidGetAppVersionString();
void androidShareDialog(const std::string &message,
						const std::string &path,
						std::function<void(bool)> completionCallback);
void androidImageDialog(const std::string &copyToPath,
						std::function<void(bool success, std::string imgPath)> completionCallback);
void androidFileDialog(std::string copyToPath,
					   const std::vector<std::string> &allowedExtensions,
					   std::function<void(std::string resultingPath, bool success)> completionCallback);

void androidLaunchUrl(const std::string &url);

void androidDisplayHtml(const std::string &html);
void androidDisplayHtmlFile(const std::string &htmlFilePath);
void androidStopDisplayingHtml();
void androidCallJs(const std::string js);
void registerWebViewOverlay(std::uintptr_t identifier, const std::function<void(const std::string &)> &jsCallback);
void unregisterWebViewOverlay(std::uintptr_t identifier);
void notifyJSCallbacks(const std::string &jsValue);

bool androidEncodeAAC(const std::string &pathToOutput, const FloatBuffer &buff, int numChannels, int sampleRate);

std::string getAndroidTempDir();

bool androidIsOnWifi();

bool isUsingHeadphones();
bool isUsingUSBInterface();
bool isUsingAirplay();
bool isUsingBluetoothHeadphones();
class AllMidiDevicesAndroidImpl;

void androidSetMainThreadRunner(MainThreadRunner *runner);
void androidSetupMidiManager(std::shared_ptr<AllMidiDevicesAndroidImpl> impl);
void androidSendMidi(const std::vector<uint8_t> &midiData, int deviceId, int portId, std::optional<uint64_t> timestampInNanoSeconds);
void androidDisplayMidiBLEPanel();

[[nodiscard]] std::vector<AudioPort> getAudioInputPorts();
[[nodiscard]] std::vector<AudioPort> getBuiltInAudioInputPorts();
[[nodiscard]] std::vector<AudioPort> getAllAudioInputPorts();
[[nodiscard]] bool hasExternalAudioInputs();
[[nodiscard]] std::string getAudioInputName(int32_t deviceId);
[[nodiscard]] int getAudioInputId(const std::string &name);

android_app *getAndroidAppPtr();
class EventDispatcher;
EventDispatcher *getAndroidEventDispatcher();
void callJNI(const std::string &methodName,
			 const std::string &arg1,
			 const std::string &arg2,
			 const std::string &arg3,
			 const std::string &arg4,
			 const std::string &arg5);
void callJNI(const std::string &methodName,
			 const std::string &arg1,
			 const std::string &arg2,
			 const std::string &arg3,
			 const std::string &arg4);
void callJNI(const std::string &methodName,
			 const std::string &arg1,
			 const std::string &arg2,
			 const std::string &arg3);
void callJNI(const std::string &methodName, const std::string &arg1, const std::string &arg2);
void callJNI(const std::string &methodName, const std::string &arg1);
void callJNI(const std::string &methodName);
void callJNI(const std::string &methodName, int32_t arg1);
void callJNI(const std::string &methodName, float arg1);

bool callJNIForBoolean(const std::string &methodName);
bool callJNIForBoolean(const std::string &methodName, int arg1);
bool callJNIForBoolean(const std::string &methodName, const std::string &arg1);
bool callJNIForBoolean(const std::string &methodName, const std::string &arg1, int arg2);
bool callJNIForBoolean(const std::string &methodName, const std::string &arg1, const std::string &arg2);

void callJNIForStringArray(const std::string &methodName, std::vector<std::string> &data);

float callJNIForFloat(const std::string &methodName);

int64_t callJNIForLong(const std::string &methodName);
int32_t callJNIForInt(const std::string &methodName);
int32_t callJNIForInt(const std::string &methodName, int32_t arg1);
int32_t callJNIForInt(const std::string &methodName, const std::string &arg1);

std::string callJNIForString(const std::string &methodName, int32_t arg1);
int64_t callJNIForLong(const std::string &methodName, int32_t arg1);

void callJNI(const std::string &methodName, const std::string &arg1, int32_t arg2);

std::string callJNIForString(const std::string &methodName);
std::string callJNIForString(const std::string &methodName, const std::string &arg1);
std::string jstringToString(JNIEnv *jni, jstring text);

class ScopedJni {
public:
    ScopedJni() {
        mzAssert(jni == nullptr);
        if (auto *appPtr = getAndroidAppPtr()) {
            if (appPtr->activity->vm->GetEnv(reinterpret_cast<void**>(&jni), JNI_VERSION_1_6) != JNI_OK) {
                success = appPtr->activity->vm->AttachCurrentThread(&jni, nullptr) == JNI_OK;
                attached = success;
            } else {
                success = true;
                attached = false;
            }
        }
    }

    ~ScopedJni() {
        if (success && attached) {
            if (auto *appPtr = getAndroidAppPtr()) {
                appPtr->activity->vm->DetachCurrentThread();
            }
            jni = nullptr;
        }
    }

    [[nodiscard]] JNIEnv *operator->() { return j(); }
    JNIEnv *j() { return jni; }

    jmethodID getMethodID(const std::string &methodName, const std::string &signature) {
        if (auto *cl = getClass()) {
            return jni->GetMethodID(cl, methodName.c_str(), signature.c_str());
        }
        return nullptr;
    }

    jclass getClass() {
        if (!success) {
            return nullptr;
        }
        if (auto *appPtr = getAndroidAppPtr()) {
            return (jclass)jni->GetObjectClass(appPtr->activity->clazz);;
        }
        return nullptr;
    }

private:
    bool success = false;
    bool attached = false;
    JNIEnv *jni = nullptr;
};
