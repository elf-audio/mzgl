
#include "androidUtil.h"
#include <functional>
#include <string>
#include <util/log.h>
#include "util.h"
#include "mainThread.h"

#define PCM_OUT_BUFF_SIZE (32 * 1024)

using namespace std;
bool ScopedJniAttachmentBlocker::shouldBlock = false;

struct android_statics {
	std::function<void()> okPressed;
	std::function<void()> buttonOnePressed;
	std::function<void()> buttonTwoPressed;
	std::function<void()> buttonThreePressed;

	std::function<void()> cancelPressed;
	std::function<void(bool)> shareCompleteCallback;
	function<void(string, bool)> completionCallback;
    std::function<void(bool success, string imgPath)> imgDialogCallback;
    std::function<void(string filePath, bool success)> fileDialogCallback;
} android_statics;

bool androidIsOnWifi() {
	return callJNIForBoolean("isOnWifi");
}

bool isUsingHeadphones() {
	return callJNIForBoolean("isUsingHeadphones");
}

bool isUsingUSBInterface() {
	return callJNIForBoolean("isUsingUSBInterface");
}

bool androidHasMicPermission() {
	return callJNIForBoolean("hasMicPermissions");
}

bool isUsingAirplay() {
	return false;
}

bool isUsingBluetoothHeadphones() {
	return callJNIForBoolean("isUsingBluetoothHeadphones");
}

////////////////////////////////////////////////////////////////////////////
static int android_read(void *cookie, char *buf, int size) {
	return AAsset_read((AAsset *) cookie, buf, size);
}

static int android_write(void *cookie, const char *buf, int size) {
	return EACCES; // can't provide write access to the apk
}

static fpos_t android_seek(void *cookie, fpos_t offset, int whence) {
	return AAsset_seek((AAsset *) cookie, offset, whence);
}

static int android_close(void *cookie) {
	AAsset_close((AAsset *) cookie);
	return 0;
}

FILE *android_asset_open(const char *fname, const char *mode) {
	if (mode[0] == 'w') return NULL;

	AAsset *asset = AAssetManager_open(getAndroidAppPtr()->activity->assetManager, fname, 0);
	if (!asset) return NULL;

	return funopen(asset, android_read, android_write, android_seek, android_close);
}

std::vector<std::string> androidListAssetDir(const std::string &path) {
	std::vector<std::string> filenames;
	auto *dir = AAssetManager_openDir(getAndroidAppPtr()->activity->assetManager, path.c_str());
	while (const char *nextFile = AAssetDir_getNextFileName(dir)) {
		filenames.emplace_back(nextFile);
	}

	AAssetDir_close(dir);
	return filenames;
}
////////////////////////////////////////////////////////////////////////////

std::vector<std::string> androidGetMidiDeviceNames() {
	if (getAndroidAppPtr() == nullptr) return std::vector<std::string>();
	std::vector<std::string> outDevs;
	JNIEnv *jni;
	int success = getAndroidAppPtr()->activity->vm->AttachCurrentThread(&jni, nullptr);
	if (success != JNI_OK) {
		Log::e() << "Got a problem with androidGetMidiDeviceNames";
	}

	jclass clazz = jni->GetObjectClass(getAndroidAppPtr()->activity->clazz);

	jmethodID methodID = jni->GetMethodID(clazz, "getMidiPorts", "()[Lcom.elf.MidiPort;");

	jobjectArray res = (jobjectArray) jni->CallObjectMethod(getAndroidAppPtr()->activity->clazz, methodID);

	int numDevs = jni->GetArrayLength(res);
	for (int i = 0; i < numDevs; i++) {
		jobject dev		= jni->GetObjectArrayElement(res, i);
		jclass devClass = jni->GetObjectClass(dev);
		jstring nameStr =
			(jstring) jni->GetObjectField(dev, jni->GetFieldID(devClass, "name", "Ljava.lang.String;"));
		outDevs.push_back(jstringToString(jni, nameStr));
	}

	getAndroidAppPtr()->activity->vm->DetachCurrentThread();
	return outDevs;
}

std::string androidGetAppVersionString() {
	return callJNIForString("getAppVersionString");
}
std::string loadAndroidAssetAsString(const std::string &path) {
	std::vector<unsigned char> outData;
	if (loadAndroidAsset(path, outData)) {
		return std::string(outData.begin(), outData.end());
	}
	return "";
}

bool loadAndroidAsset(const std::string &path, std::vector<unsigned char> &outData) {
	if (getAndroidAppPtr() == nullptr) return false;

	//LOGE("loadAndroidAsset('%s')", path.c_str());
	AAsset *pAsset =
		AAssetManager_open(getAndroidAppPtr()->activity->assetManager, path.c_str(), AASSET_MODE_BUFFER);
	if (pAsset == nullptr) {
		LOGE("Can't open %s", path.c_str());
		return false;
	} else {
		size_t length		= AAsset_getLength(pAsset);
		unsigned char *data = (unsigned char *) AAsset_getBuffer(pAsset);
		if (data == nullptr) {
			LOGE("ERROR READING");
			AAsset_close(pAsset);
			return false;
		}
		//LOGE("About to copy data - data length: %d\n", length);
		// POSSIBLE BUG HERE - DO I NEED TO FREE data?
		outData.insert(outData.end(), data, data + length);
		//LOGE("Closing asset\n");
		AAsset_close(pAsset);
		//LOGE("Copied data");
		return true;
	}
}

void listAndroidAssetDir(const std::string &path, vector<std::string> &outPaths) {
	if (getAndroidAppPtr() == nullptr) return;

	AAssetDir *assetDir = AAssetManager_openDir(getAndroidAppPtr()->activity->assetManager, path.c_str());
	const char *filePath;
	while ((filePath = AAssetDir_getNextFileName(assetDir)) != nullptr) {
		outPaths.emplace_back(filePath);
	}
}

std::string getAndroidTempDir() {
	return callJNIForString("getTempDir");
}

std::string getAndroidInternalDataPath() {
	if (getAndroidAppPtr() == nullptr) return "";
	return std::string(getAndroidAppPtr()->activity->internalDataPath);
}

std::string getAndroidExternalDataPath() {
	if (getAndroidAppPtr() == nullptr) return "";
	return std::string(getAndroidAppPtr()->activity->externalDataPath);
}

std::string getAndroidExternalStorageDirectory() {
	return callJNIForString("getExternalStorageDirectory");
}

void androidLaunchUrl(const std::string &url) {
	callJNI("launchUrl", url);
}

void androidDisplayHtml(const std::string &html) {
	callJNI("displayHtml", html);
}

bool androidEncodeAAC(std::string pathToOutput, const FloatBuffer &inputBuffer, int numChannels, int sampleRate) {
	ScopedJni scp;
	jmethodID methodID = scp.getMethodID("getAACEncoder", "()Lcom/elf/aacencoder/Encoder;");
	JNIEnv *jni		   = scp.j();
	bool success	   = false;

	jobject aacEncoder = jni->CallObjectMethod(getAndroidAppPtr()->activity->clazz, methodID);
	if (aacEncoder != nullptr) {
		jclass aacEncoderClazz = jni->GetObjectClass(aacEncoder);
		jmethodID initMethodID = jni->GetMethodID(aacEncoderClazz, "initialize", "(Ljava/lang/String;II)Z");
		jmethodID feedMethodID = jni->GetMethodID(aacEncoderClazz, "feed", "(Ljava/nio/ByteBuffer;I)Z");
		jmethodID doneMethodID = jni->GetMethodID(aacEncoderClazz, "done", "()V");
		jobject nativeBuffer   = nullptr;
		jstring path		   = jni->NewStringUTF(pathToOutput.c_str());

		jboolean initOK = jni->CallBooleanMethod(aacEncoder, initMethodID, path, numChannels, sampleRate);
		if (initOK == JNI_TRUE) {
			// unfortunately android AAC encoder needs signed int 16 sample as input
			// create reasonably large buffer

			uint32_t numberOfPCMSamples = inputBuffer.size();
			uint32_t shortBuffSize =
				(numberOfPCMSamples > PCM_OUT_BUFF_SIZE) ? PCM_OUT_BUFF_SIZE : numberOfPCMSamples;
			uint32_t byteBuffSize = shortBuffSize * 2;

			auto *byteBuffer = new uint8_t[byteBuffSize];
			nativeBuffer	 = jni->NewDirectByteBuffer(byteBuffer, byteBuffSize);

			uint32_t dataLeft = (nativeBuffer != nullptr) ? numberOfPCMSamples : 0;

			int inputBuffPos = 0;
			jboolean feedOK	 = JNI_FALSE;

			while (dataLeft > 0) {
				uint32_t samplesToCopy = (dataLeft > shortBuffSize) ? shortBuffSize : dataLeft;
				uint32_t outBufPos	   = 0;
				for (int i = 0; i < samplesToCopy; i++) {
					auto signedSample =
						(int16_t) (inputBuffer[inputBuffPos++] * 32767.f); // mind the endian, which must be LE
					byteBuffer[outBufPos++] = (uint8_t) signedSample;
					byteBuffer[outBufPos++] = (uint8_t) (signedSample >> 8);
				}
				dataLeft -= samplesToCopy;

				feedOK = jni->CallBooleanMethod(aacEncoder, feedMethodID, nativeBuffer, samplesToCopy * 2);
				if (feedOK != JNI_TRUE) {
					break;
				}
			}
			jni->CallVoidMethod(aacEncoder, doneMethodID);

			if (nativeBuffer != nullptr) {
				jni->DeleteLocalRef(nativeBuffer);
			}

			success = (feedOK == JNI_TRUE);
		}
		jni->DeleteLocalRef(path);
		jni->DeleteLocalRef(aacEncoder);
	}

	return success;
}

void androidAlertDialog(const std::string &title, const std::string &message) {
	callJNI("alertDialog", title, message);
}

void androidConfirmDialog(std::string title,
						  std::string msg,
						  std::function<void()> okPressed,
						  std::function<void()> cancelPressed) {
	android_statics.okPressed	  = okPressed;
	android_statics.cancelPressed = cancelPressed;
	callJNI("confirmDialog", title, msg);
}

void androidTwoOptionCancelDialog(std::string title,
								  std::string msg,
								  std::string buttonOneText,
								  std::function<void()> buttonOnePressed,
								  std::string buttonTwoText,
								  std::function<void()> buttonTwoPressed,
								  std::function<void()> cancelPressed) {
	android_statics.buttonOnePressed = buttonOnePressed;
	android_statics.buttonTwoPressed = buttonTwoPressed;
	android_statics.cancelPressed	 = cancelPressed;
	callJNI("twoOptionCancelDialog", title, msg, buttonOneText, buttonTwoText);
}

void androidThreeOptionCancelDialog(std::string title,
									std::string msg,
									std::string buttonOneText,
									std::function<void()> buttonOnePressed,
									std::string buttonTwoText,
									std::function<void()> buttonTwoPressed,
									std::string buttonThreeText,
									std::function<void()> buttonThreePressed,
									std::function<void()> cancelPressed) {
	android_statics.buttonOnePressed   = buttonOnePressed;
	android_statics.buttonTwoPressed   = buttonTwoPressed;
	android_statics.buttonThreePressed = buttonThreePressed;
	android_statics.cancelPressed	   = cancelPressed;
	callJNI("threeOptionCancelDialog", title, msg, buttonOneText, buttonTwoText, buttonThreeText);
}

void androidImageDialog(std::string copyToPath,
						std::function<void(bool success, string imgPath)> completionCallback) {
	// TODO: safe callback to android_statics
	android_statics.imgDialogCallback = completionCallback;
	callJNI("imageDialog", copyToPath);
}
static std::string csv(const std::vector<std::string>& vec) {
    std::string result;
    for(size_t i = 0; i < vec.size(); ++i) {
        result += vec[i];
        if (i != vec.size() - 1) { // Check so we don't add a comma after the last element
            result += ", ";
        }
    }
    return result;
}
void androidFileDialog(std::string copyToPath,
                       const std::vector<std::string> &allowedExtensions,
                       std::function<void(std::string resultingPath, bool success)> completionCallback) {
	android_statics.fileDialogCallback = completionCallback;
    callJNI("fileDialog", csv(allowedExtensions));
}


void androidShareDialog(std::string message, std::string path, std::function<void(bool)> completionCallback) {
	android_statics.shareCompleteCallback = completionCallback;
	callJNI("shareDialog", message, path);
}

void androidTextboxDialog(std::string title,
						  std::string msg,
						  std::string text,
						  function<void(string, bool)> completionCallback) {
	android_statics.completionCallback = completionCallback;
	callJNI("textboxDialog", title, msg, text);
}

string jstringToString(JNIEnv *jni, jstring text) {
	const char *nativeString = jni->GetStringUTFChars(text, nullptr);
	string s				 = string(nativeString);
	jni->ReleaseStringUTFChars(text, nativeString);
	return s;
}

static std::weak_ptr<AllMidiDevicesAndroidImpl> midiImpl;
void androidSetupAllMidiIns(std::shared_ptr<AllMidiDevicesAndroidImpl> impl) {
    midiImpl = impl;
	Log::d() << "About to call allmidiIns";
	callJNI("setupAllMidiIns");
	Log::d() << "Finished setting up all midi ins";
}

vector<unsigned char> currMsg;

// eventually, we need to actually enumerate android deviced but this not done yet.
MidiDevice androidMidiDeviceChangeMe;
#include "AllMidiDevicesAndroidImpl.h"

void androidEmitMidiMessage(const vector<unsigned char> &msg, uint64_t timestamp) {
	// TODO: Does this need to run on audio thread?
    if(auto impl = midiImpl.lock()) {
        impl->messageReceived(androidMidiDeviceChangeMe, {msg}, timestamp);
    } else {
        Log::e() << "ERROR: android allMidiDevices not assigned";
    }
}

// BE CAREFUL HERE - THIS IS A DUPLICATION OF KOALAMIDIIN PARSER LOGIC
void androidParseMidiData(vector<unsigned char> d, uint64_t timestamp) {
	// so find the first message that has a 1 at the front
	for (int i = 0; i < d.size(); i++) {
		//printBinary(d[i]);
		if (d[i] & 0x80) {
			// this is a status byte, send any previous messages
			if (currMsg.size() > 0) {
				androidEmitMidiMessage(currMsg, timestamp);
				currMsg.clear();
			}
			currMsg.push_back(d[i]);
			// if this status byte is for a 1 byte message send it straight away
			// all status 0xF6 and above are all the 1 byte messages.
			if (currMsg.size() == 1 && currMsg[0] >= 0xF6) {
				androidEmitMidiMessage(currMsg, timestamp);
				currMsg.clear();
			}

		} else {
			// a data byte

			// first check we have a status in the gun, otherwise can this message
			if (currMsg.size() == 0 || (currMsg[0] & 0x80) == 0) {
				currMsg.clear();
			} else {
				currMsg.push_back(d[i]);
				// now check to see if the message is finished
				// by seeing which status we have and looking it
				// up in known statuses/lengths
				int status		= currMsg[0] & 0xF0;
				bool shouldEmit = false;
				switch (status) {
					case MIDI_NOTE_OFF:
					case MIDI_NOTE_ON:
					case MIDI_PITCH_BEND:
					case MIDI_POLY_AFTERTOUCH:
					case MIDI_SONG_POS_POINTER:
					case MIDI_CONTROL_CHANGE:
						if (currMsg.size() == 3) shouldEmit = true;
						break;
					case MIDI_PROGRAM_CHANGE:
					case MIDI_AFTERTOUCH:
						if (currMsg.size() == 2) shouldEmit = true;
						break;

					default: // status unknown, don't send, next message will push this through
						break;
				}
				if (shouldEmit) {
					androidEmitMidiMessage(currMsg, timestamp);
					currMsg.clear();
				}
			}
		}
	}
}

#ifdef __cplusplus
extern "C" {
#endif

JNIEXPORT void JNICALL Java_com_elf_MZGLActivity_midiReceived(JNIEnv *env,
															  jobject thiz,
															  jbyteArray bytes,
															  jlong timestamp) {
	// TODO: implement midiReceived()
	// just assume it's a single message
	jboolean isCopy;
	int size = env->GetArrayLength(bytes);
	jbyte *b = env->GetByteArrayElements(bytes, &isCopy);
	vector<unsigned char> data(size);
	for (int i = 0; i < size; i++) {
		data[i] = ((unsigned char *) b)[i];
	}
	env->ReleaseByteArrayElements(bytes, b, 0);

	// now we need to split the message(s)
	androidParseMidiData(data, timestamp);
}

JNIEXPORT void JNICALL Java_com_elf_MZGLActivity_okPressed(JNIEnv *, jobject) {
	androidGetApp()->main.runOnMainThread([]() { android_statics.okPressed(); });
}

JNIEXPORT void JNICALL Java_com_elf_MZGLActivity_cancelPressed(JNIEnv *, jobject) {
	androidGetApp()->main.runOnMainThread([]() { android_statics.cancelPressed(); });
}

JNIEXPORT void JNICALL Java_com_elf_MZGLActivity_button1Pressed(JNIEnv *, jobject) {
	androidGetApp()->main.runOnMainThread([]() { android_statics.buttonOnePressed(); });
}

JNIEXPORT void JNICALL Java_com_elf_MZGLActivity_button2Pressed(JNIEnv *, jobject) {
	androidGetApp()->main.runOnMainThread([]() { android_statics.buttonTwoPressed(); });
}

JNIEXPORT void JNICALL Java_com_elf_MZGLActivity_button3Pressed(JNIEnv *, jobject) {
	androidGetApp()->main.runOnMainThread([]() { android_statics.buttonThreePressed(); });
}

JNIEXPORT void JNICALL Java_com_elf_MZGLActivity_imageDialogComplete(JNIEnv *env,
																	 jobject thiz,
																	 jboolean success,
																	 jstring img_path) {
	string path = jstringToString(env, img_path);
	bool succ	= success;

	androidGetApp()->main.runOnMainThread([succ, path]() { android_statics.imgDialogCallback(succ, path); });
}


JNIEXPORT void JNICALL Java_com_elf_MZGLActivity_importFileComplete(JNIEnv *env,
																	 jobject thiz,
																	 jboolean success,
																	 jstring filePath) {
	string path = jstringToString(env, filePath);
	bool succ	= success;

	androidGetApp()->main.runOnMainThread([succ, path]() {
		android_statics.fileDialogCallback(path, succ);
	});
}


JNIEXPORT void JNICALL Java_com_elf_MZGLActivity_textboxDialogComplete(JNIEnv *jni,
																	   jobject,
																	   jboolean success,
																	   jstring text) {
	string s  = jstringToString(jni, text);
	bool succ = success;
	androidGetApp()->main.runOnMainThread([s, succ]() { android_statics.completionCallback(s, succ); });
}

#ifdef __cplusplus
}
#endif
