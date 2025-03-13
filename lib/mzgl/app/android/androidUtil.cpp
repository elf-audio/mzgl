
#include "androidUtil.h"
#include <functional>
#include <string>
#include <util/log.h>
#include "util.h"
#include "mainThread.h"

#define PCM_OUT_BUFF_SIZE (32 * 1024)

using namespace std;
//bool ScopedJniAttachmentBlocker::shouldBlock = false;

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

#if !defined(__APPLE__)

void quitApplication() {
    mzAssert(false);
}

#endif

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

    AAsset *pAsset =
            AAssetManager_open(getAndroidAppPtr()->activity->assetManager, path.c_str(),
                               AASSET_MODE_BUFFER);
    if (pAsset == nullptr) {
        LOGE("Can't open %s", path.c_str());
        return false;
    } else {
        size_t length = AAsset_getLength(pAsset);
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

    AAssetDir *assetDir = AAssetManager_openDir(getAndroidAppPtr()->activity->assetManager,
                                                path.c_str());
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

void androidDisplayHtmlFile(const std::string &htmlFilePath) {
    callJNI("displayHtmlFile", htmlFilePath);
}

void androidCallJs(const std::string js) {
    callJNI("callJs", js);
}

void androidStopDisplayingHtml() {
    callJNI("stopDisplayingHtml");
}

std::map<std::string, FileDownloaderCallbacks> &getDownloadFiles() {
    static std::map<std::string, FileDownloaderCallbacks> downloadFiles;
    return downloadFiles;
}

std::mutex &getDownloadFilesMutex() {
    static std::mutex downloadFilesMutex;
    return downloadFilesMutex;
}

void registerDownload(const std::string &url, const FileDownloaderCallbacks &callbacks) {
    std::lock_guard lock(getDownloadFilesMutex());
    auto &downloadFiles = getDownloadFiles();
#ifdef DEBUG
    auto iter = downloadFiles.find(url);
    if (iter != std::end(downloadFiles)) {
        mzAssert(false);
    }
#endif
    downloadFiles[url] = callbacks;
}

void unregisterDownload(const std::string &url) {
    std::lock_guard lock(getDownloadFilesMutex());
    auto &downloadFiles = getDownloadFiles();
    auto iter = downloadFiles.find(url);
    if (iter == std::end(downloadFiles)) {
        mzAssert(false);
        return;
    }
    downloadFiles.erase(iter);
}

void notifyFileDownloadStarted(const std::string &url) {
    std::lock_guard lock(getDownloadFilesMutex());
    auto &downloadFiles = getDownloadFiles();
    auto iter = downloadFiles.find(url);
    if (iter == std::end(downloadFiles)) {
        Log::d() << "Failed to find download for " << url << " during start notification";
        return;
    }
}

void notifyFileDownloadComplete(const std::string &url, const std::string &path) {
    std::lock_guard lock(getDownloadFilesMutex());
    auto &downloadFiles = getDownloadFiles();
    auto iter = downloadFiles.find(url);
    if (iter == std::end(downloadFiles)) {
        Log::d() << "Failed to find download for " << url << " during complete notification";
        return;
    }
    if (iter->second.downloadCompleted != nullptr) {
        iter->second.downloadCompleted(path);
    }
}

void notifyFileDownloadFailed(const std::string &url, const std::string &reason) {
    std::lock_guard lock(getDownloadFilesMutex());
    auto &downloadFiles = getDownloadFiles();
    auto iter = downloadFiles.find(url);
    if (iter == std::end(downloadFiles)) {
        Log::d() << "Failed to find download for " << url << " during failed notification";
        return;
    }
    if (iter->second.downloadFailed != nullptr) {
        iter->second.downloadFailed(reason);
    }
}

void notifyFileDownloadCancelled(const std::string &url) {
    std::lock_guard lock(getDownloadFilesMutex());
    auto &downloadFiles = getDownloadFiles();
    auto iter = downloadFiles.find(url);
    if (iter == std::end(downloadFiles)) {
        Log::d() << "Failed to find download for " << url << " during cancelled notification";
        return;
    }
    if (iter->second.downloadCancelled != nullptr) {
        iter->second.downloadCancelled();
    }
}

void notifyFileDownloadProgress(const std::string &url, float progress) {
    std::lock_guard lock(getDownloadFilesMutex());
    auto &downloadFiles = getDownloadFiles();
    auto iter = downloadFiles.find(url);
    if (iter == std::end(downloadFiles)) {
        Log::d() << "Failed to find download for " << url << " during progress notification";
        return;
    }
    if (iter->second.downloadProgressChanged != nullptr) {
        iter->second.downloadProgressChanged(progress);
    }
}

void androidDownloadFile(const std::string &url,
                         const std::string &outputPath,
                         const FileDownloaderCallbacks &callbacks) {
    registerDownload(url, callbacks);
    callJNI("downloadFile", url, outputPath);
}

void androidCancelFileDownload(const std::string &url) {
    callJNI("cancelFileDownload", url);
}

std::map<std::uintptr_t, std::function<void(const std::string &)>> webViews;

void registerWebViewOverlay(std::uintptr_t identifier,
                            const std::function<void(const std::string &)> &jsCallback) {
    auto iter = webViews.find(identifier);
    if (iter != std::end(webViews)) {
        mzAssert(false);
        webViews.erase(iter);
    }

    webViews.insert({identifier, jsCallback});
}

void unregisterWebViewOverlay(std::uintptr_t identifier) {
    auto iter = webViews.find(identifier);
    if (iter == std::end(webViews)) {
        mzAssert(false);
        return;
    }
    webViews.erase(iter);
}

void notifyJSCallbacks(const std::string &jsValue) {
    if (webViews.empty() && jsValue == "close") {
        androidStopDisplayingHtml();
    }
    auto views = webViews;
    for (auto &[id, callback]: views) {
        callback(jsValue);
    }
}

bool androidEncodeAAC(const std::string &pathToOutput,
                      const FloatBuffer &inputBuffer,
                      int numChannels,
                      int sampleRate) {
    ScopedJni scp;
    jmethodID methodID = scp.getMethodID("getAACEncoder", "()Lcom/elf/aacencoder/Encoder;");
    JNIEnv *jni = scp.j();
    bool success = false;

    jobject aacEncoder = jni->CallObjectMethod(getAndroidAppPtr()->activity->clazz, methodID);
    if (aacEncoder != nullptr) {
        jclass aacEncoderClazz = jni->GetObjectClass(aacEncoder);
        jmethodID initMethodID = jni->GetMethodID(aacEncoderClazz, "initialize",
                                                  "(Ljava/lang/String;II)Z");
        jmethodID feedMethodID = jni->GetMethodID(aacEncoderClazz, "feed",
                                                  "(Ljava/nio/ByteBuffer;I)Z");
        jmethodID doneMethodID = jni->GetMethodID(aacEncoderClazz, "done", "()V");
        jobject nativeBuffer = nullptr;
        jstring path = jni->NewStringUTF(pathToOutput.c_str());

        jboolean initOK = jni->CallBooleanMethod(aacEncoder, initMethodID, path, numChannels,
                                                 sampleRate);
        if (initOK == JNI_TRUE) {
            // unfortunately android AAC encoder needs signed int 16 sample as input
            // create reasonably large buffer

            uint32_t numberOfPCMSamples = inputBuffer.size();
            uint32_t shortBuffSize =
                    (numberOfPCMSamples > PCM_OUT_BUFF_SIZE) ? PCM_OUT_BUFF_SIZE
                                                             : numberOfPCMSamples;
            uint32_t byteBuffSize = shortBuffSize * 2;

            auto *byteBuffer = new uint8_t[byteBuffSize];
            nativeBuffer = jni->NewDirectByteBuffer(byteBuffer, byteBuffSize);

            uint32_t dataLeft = (nativeBuffer != nullptr) ? numberOfPCMSamples : 0;

            int inputBuffPos = 0;
            jboolean feedOK = JNI_FALSE;

            while (dataLeft > 0) {
                uint32_t samplesToCopy = (dataLeft > shortBuffSize) ? shortBuffSize : dataLeft;
                uint32_t outBufPos = 0;
                for (int i = 0; i < samplesToCopy; i++) {
                    auto signedSample =
                            (int16_t) (inputBuffer[inputBuffPos++] *
                                       32767.f); // mind the endian, which must be LE
                    byteBuffer[outBufPos++] = (uint8_t) signedSample;
                    byteBuffer[outBufPos++] = (uint8_t) (signedSample >> 8);
                }
                dataLeft -= samplesToCopy;

                feedOK = jni->CallBooleanMethod(aacEncoder, feedMethodID, nativeBuffer,
                                                samplesToCopy * 2);
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

void androidConfirmDialog(const std::string &title,
                          const std::string &msg,
                          std::function<void()> okPressed,
                          std::function<void()> cancelPressed) {
    android_statics.okPressed = okPressed;
    android_statics.cancelPressed = cancelPressed;
    callJNI("confirmDialog", title, msg);
}

void androidTwoOptionDialog(const std::string &title,
                            const std::string &msg,
                            const std::string &buttonOneText,
                            std::function<void()> buttonOnePressed,
                            const std::string &buttonTwoText,
                            std::function<void()> buttonTwoPressed) {
    android_statics.buttonOnePressed = std::move(buttonOnePressed);
    android_statics.buttonTwoPressed = std::move(buttonTwoPressed);
    callJNI("twoOptionDialog", title, msg, buttonOneText, buttonTwoText);
}

void androidTwoOptionCancelDialog(const std::string &title,
                                  const std::string &msg,
                                  const std::string &buttonOneText,
                                  std::function<void()> buttonOnePressed,
                                  const std::string &buttonTwoText,
                                  std::function<void()> buttonTwoPressed,
                                  std::function<void()> cancelPressed) {
    android_statics.buttonOnePressed = buttonOnePressed;
    android_statics.buttonTwoPressed = buttonTwoPressed;
    android_statics.cancelPressed = cancelPressed;
    callJNI("twoOptionCancelDialog", title, msg, buttonOneText, buttonTwoText);
}

void androidThreeOptionCancelDialog(const std::string &title,
                                    const std::string &msg,
                                    const std::string &buttonOneText,
                                    std::function<void()> buttonOnePressed,
                                    const std::string &buttonTwoText,
                                    std::function<void()> buttonTwoPressed,
                                    const std::string &buttonThreeText,
                                    std::function<void()> buttonThreePressed,
                                    std::function<void()> cancelPressed) {
    android_statics.buttonOnePressed = buttonOnePressed;
    android_statics.buttonTwoPressed = buttonTwoPressed;
    android_statics.buttonThreePressed = buttonThreePressed;
    android_statics.cancelPressed = cancelPressed;
    callJNI("threeOptionCancelDialog", title, msg, buttonOneText, buttonTwoText, buttonThreeText);
}

void androidImageDialog(const std::string &copyToPath,
                        std::function<void(bool success, string imgPath)> completionCallback) {
    // TODO: safe callback to android_statics
    android_statics.imgDialogCallback = completionCallback;
    callJNI("imageDialog", copyToPath);
}

static std::string csv(const std::vector<std::string> &vec) {
    std::string result;
    for (size_t i = 0; i < vec.size(); ++i) {
        result += vec[i];
        if (i != vec.size() - 1) { // Check so we don't add a comma after the last element
            result += ", ";
        }
    }
    return result;
}

void androidFileDialog(std::string copyToPath,
                       const std::vector<std::string> &allowedExtensions,
                       std::function<void(std::string resultingPath,
                                          bool success)> completionCallback) {
    android_statics.fileDialogCallback = completionCallback;
    callJNI("fileDialog", csv(allowedExtensions));
}

void androidShareDialog(const std::string &message,
                        const std::string &path,
                        std::function<void(bool)> completionCallback) {
    android_statics.shareCompleteCallback = completionCallback;
    callJNI("shareDialog", message, path);
}

void androidTextboxDialog(const std::string &title,
                          const std::string &msg,
                          const std::string &text,
                          function<void(string, bool)> completionCallback) {
    android_statics.completionCallback = completionCallback;
    callJNI("textboxDialog", title, msg, text);
}

void androidNumberboxDialog(const std::string &title,
                            const std::string &msg,
                            const std::string &initialValue,
                            std::function<void(std::string, bool)> completionCallback) {
    android_statics.completionCallback = completionCallback;
    callJNI("numberboxDialog", title, msg, initialValue);
}

string jstringToString(JNIEnv *jni, jstring text) {
    const char *nativeString = jni->GetStringUTFChars(text, nullptr);
    string s = string(nativeString);
    jni->ReleaseStringUTFChars(text, nativeString);
    return s;
}

#include "AllMidiDevicesAndroidImpl.h"
#include "MidiMessageParser.h"

std::vector<AudioPort> convertAudioPortFromNames(const std::vector<std::string> &audioPortNames) {
    AudioPort basicPort{-1, 0, 0, 44100.0, {}, "EMPTY-PORT", false, false};

    std::vector<AudioPort> ports;
    for (auto name: audioPortNames) {
        basicPort.name = name;
        ports.push_back(basicPort);
    }

    return ports;
}

std::vector<AudioPort> getAudioInputPorts(const std::string &accessorName) {
    std::vector<std::string> audioPortNames;
    callJNIForStringArray(accessorName, audioPortNames);
    return convertAudioPortFromNames(audioPortNames);
}

std::vector<AudioPort> getAudioInputPorts() {
    return getAudioInputPorts("getAudioInputs");
}

std::vector<AudioPort> getBuiltInAudioInputPorts() {
    return getAudioInputPorts("getBuiltInAudioInputs");
}

std::vector<AudioPort> getAllAudioInputPorts() {
    auto inputs = getAudioInputPorts();
    auto builtIn = getBuiltInAudioInputPorts();
    inputs.insert(std::end(inputs), std::begin(builtIn), std::end(builtIn));
    return inputs;
}

bool hasExternalAudioInputs() {
    return callJNIForBoolean("hasExternalAudioInputs");
}

std::string getAudioInputName(int32_t deviceId) {
    return callJNIForString("getAudioInputName", deviceId);
}

int getAudioInputId(const std::string &name) {
    return callJNIForInt("getAudioInputId", name);
}

static std::weak_ptr<AllMidiDevicesAndroidImpl> midiImpl;

void androidSetupMidiManager(std::shared_ptr<AllMidiDevicesAndroidImpl> impl) {
    midiImpl = impl;
    Log::d() << "About to call setupMidiManager";
    callJNI("setupMidiManager");
    Log::d() << "Finished setting up MidiManager";
}

void androidSetMainThreadRunner(MainThreadRunner *runner) {
    if (auto impl = midiImpl.lock()) {
        impl->setMainThreadRunner(runner);
    }
}

#include "concurrentqueue.h"

class AndroidMidiThread {
public:
    AndroidMidiThread() { start(); }

    ~AndroidMidiThread() {
        shouldContinueProcessing.store(false);
        thread.join();
    }

    struct MidiByteMessage {
        std::vector<uint8_t> bytes;
        int deviceId;
        int portId;
        std::optional<uint64_t> timestampInNanoSeconds;
    };

    void send(const MidiByteMessage &message) { messages.enqueue(message); }

private:
    class Bytes {
    public:
        JNIEnv *jni;
        jbyteArray midiBytes;
        jbyteArray kotlinMidiBytes;

        void cleanup() {
            jni->DeleteLocalRef(kotlinMidiBytes);
            jni->DeleteLocalRef(midiBytes);
        }
    };

    void start() {
        thread = std::thread([this]() {
            jni = std::make_unique<ScopedJni>();
            auto methodID = jni->getMethodID("sendMidiData", "([BII)V");
            auto clazz = getAndroidAppPtr()->activity->clazz;
            Bytes bytes;
            bytes.jni = jni->j();

            while (shouldContinueProcessing) {
                MidiByteMessage message;
                while (messages.try_dequeue(message)) {
                    if (convert(message, bytes)) {
                        jni->j()->CallVoidMethod(
                                clazz, methodID, bytes.kotlinMidiBytes, message.deviceId,
                                message.portId);
                        bytes.cleanup();
                    }
                }

                std::this_thread::sleep_for(std::chrono::microseconds(100));
            }
        });
    }

    bool convert(const MidiByteMessage &message, Bytes &bytes) {
        const jsize sizeOfData = static_cast<jsize>(message.bytes.size());

        bytes.midiBytes = jni->j()->NewByteArray(sizeOfData);
        if (bytes.midiBytes == nullptr) {
            mzAssert(false);
            return false;
        }

        jni->j()->SetByteArrayRegion(
                bytes.midiBytes, 0, message.bytes.size(),
                reinterpret_cast<const jbyte *>(message.bytes.data()));

        jbyte *elements = jni->j()->GetByteArrayElements(bytes.midiBytes, nullptr);
        jsize length = jni->j()->GetArrayLength(bytes.midiBytes);
        bytes.kotlinMidiBytes = jni->j()->NewByteArray(length);
        if (bytes.kotlinMidiBytes == nullptr) {
            mzAssert(false);
            return false;
        }
        jni->j()->SetByteArrayRegion(bytes.kotlinMidiBytes, 0, length, elements);
        jni->j()->ReleaseByteArrayElements(bytes.midiBytes, elements, JNI_ABORT);
        return true;
    }

    std::thread thread;
    std::atomic<bool> shouldContinueProcessing{true};
    std::unique_ptr<ScopedJni> jni;
    moodycamel::ConcurrentQueue<MidiByteMessage> messages;
};

void androidSendMidi(const std::vector<uint8_t> &midiData,
                     int deviceId,
                     int portId,
                     std::optional<uint64_t> timestampInNanoSeconds) {
    static AndroidMidiThread androidMidiThread;

    androidMidiThread.send({midiData, deviceId, portId, timestampInNanoSeconds});
}

void androidDisplayMidiBLEPanel() {
    callJNI("displayMidiBLEPanel");
}

void androidOnMidiInputDeviceConnected(int32_t deviceId,
                                       int32_t portId,
                                       bool isInput,
                                       const std::string &deviceName) {
    if (auto impl = midiImpl.lock()) {
        if (isInput) {
            impl->inputDeviceConnected(
                    std::make_shared<AndroidMidiDevice>(deviceName, deviceId, portId,
                                                        AndroidMidiDevice::Type::input));
        } else {
            impl->outputDeviceConnected(std::make_shared<AndroidMidiDevice>(
                    deviceName, deviceId, portId, AndroidMidiDevice::Type::output));
        }
    }
}

void androidOnMidiInputDeviceDisconnected(int32_t deviceId, int32_t portId, bool isInput) {
    if (auto impl = midiImpl.lock()) {
        if (isInput) {
            impl->inputDeviceDisconnected(std::make_shared<AndroidMidiDevice>(
                    "DISCONNECTED", deviceId, portId, AndroidMidiDevice::Type::input));
        } else {
            impl->outputDeviceDisconnected(std::make_shared<AndroidMidiDevice>(
                    "DISCONNECTED", deviceId, portId, AndroidMidiDevice::Type::output));
        }
    }
}

void androidParseMidiData(vector<unsigned char> midiMessage,
                          uint64_t timestamp,
                          int32_t deviceId,
                          int32_t portId) {
    static std::map<std::pair<int32_t, int32_t>, MidiMessageParser> parsers;

    auto key = std::make_pair(deviceId, portId);
    auto iter = parsers.find(key);

    if (iter == std::cend(parsers)) {
        parsers.insert(std::make_pair(
                key,
                MidiMessageParser{[key, deviceId, portId](const MidiMessageParser::MidiData &data) {
                    if (auto impl = midiImpl.lock()) {
                        auto devices = impl->getConnectedMidiDevices();
                        auto deviceIter = std::find_if(
                                std::begin(devices), std::end(devices),
                                [deviceId, portId](auto &&deviceToTest) {
                                    if (auto *androidInput = dynamic_cast<AndroidMidiDevice *>(deviceToTest.get())) {
                                        return androidInput->deviceIdentifier == deviceId
                                               && androidInput->devicePort == portId &&
                                               !androidInput->isOutput;
                                    }
                                    return false;
                                });
                        if (deviceIter != std::end(devices)) {
                            impl->messageReceived(*deviceIter, MidiMessage{data.data},
                                                  data.timestamp);
                        } else {
                            Log::e() << "Didnt find device " << std::to_string(deviceId)
                                     << " and port "
                                     << std::to_string(portId);
                        }
                    }
                }}));
        iter = parsers.find(key);
    }

    iter->second.parse(midiMessage, timestamp, deviceId, portId);
}

std::vector<unsigned char> copyRawMidiData(jbyte *rawMidiData, jsize size) {
    std::vector<unsigned char> midiData(size);
    for (int i = 0; i < size; i++) {
        midiData[i] = ((unsigned char *) rawMidiData)[i];
    }
    return midiData;
}

std::vector<unsigned char> copyMidiBytes(JNIEnv *env, jbyteArray bytes) {
    jboolean isCopy;
    jsize size = env->GetArrayLength(bytes);
    jbyte *rawMidiData = env->GetByteArrayElements(bytes, &isCopy);
    auto midiData = copyRawMidiData(rawMidiData, size);
    env->ReleaseByteArrayElements(bytes, rawMidiData, 0);
    return midiData;
}

#ifdef __cplusplus
extern "C" {
#endif

JNIEXPORT void JNICALL Java_com_elf_MZGLMidiManager_deviceAdded(
        JNIEnv *env, jobject thiz, jint deviceId, jint portId, jboolean isInput,
        jstring deviceName) {
    androidOnMidiInputDeviceConnected(deviceId, portId, isInput == JNI_TRUE,
                                      jstringToString(env, deviceName));
}

JNIEXPORT void JNICALL Java_com_elf_MZGLMidiManager_deviceRemoved(
        JNIEnv *env, jobject thiz, jint deviceId, jint portId, jboolean isInput) {
    androidOnMidiInputDeviceDisconnected(deviceId, portId, isInput == JNI_TRUE);
}

JNIEXPORT void JNICALL Java_com_elf_MZGLMidiReceiver_midiReceived(
        JNIEnv *env, jobject thiz, jbyteArray bytes, jlong timestamp, jint deviceId, jint portId) {
    androidParseMidiData(copyMidiBytes(env, bytes), timestamp, deviceId, portId);
}

JNIEXPORT void JNICALL
Java_com_elf_MZGLActivity_fileDownloadStarted(JNIEnv *env, jobject thiz, jstring url) {
    androidGetApp()->main.runOnMainThread(
            [download = jstringToString(env, url)]() { notifyFileDownloadStarted(download); });
}

JNIEXPORT void JNICALL Java_com_elf_MZGLActivity_fileDownloadComplete(JNIEnv *env,
                                                                      jobject thiz,
                                                                      jstring url,
                                                                      jstring path) {
    androidGetApp()->main.runOnMainThread(
            [download = jstringToString(env, url), file = jstringToString(env, path)]() {
                notifyFileDownloadComplete(download, file);
                unregisterDownload(download);
            });
}

JNIEXPORT void JNICALL Java_com_elf_MZGLActivity_fileDownloadFailed(JNIEnv *env,
                                                                    jobject thiz,
                                                                    jstring url,
                                                                    jstring reason) {
    androidGetApp()->main.runOnMainThread(
            [download = jstringToString(env, url), error = jstringToString(env, reason)]() {
                notifyFileDownloadFailed(download, error);
                unregisterDownload(download);
            });
}

JNIEXPORT void JNICALL
Java_com_elf_MZGLActivity_fileDownloadCancelled(JNIEnv *env, jobject thiz, jstring url) {
    androidGetApp()->main.runOnMainThread(
            [download = jstringToString(env, url)]() {
                notifyFileDownloadCancelled(download);
                unregisterDownload(download);
            });
}

JNIEXPORT void JNICALL Java_com_elf_MZGLActivity_fileDownloadProgress(JNIEnv *env,
                                                                      jobject thiz,
                                                                      jstring url,
                                                                      jfloat progress) {
    androidGetApp()->main.runOnMainThread(
            [download = jstringToString(env, url), progress]() {
                notifyFileDownloadProgress(download, progress);
            });
}

JNIEXPORT void JNICALL
Java_com_elf_MZGLActivity_onJavascript(JNIEnv *env, jobject thiz, jstring javascriptValue) {
    notifyJSCallbacks(jstringToString(env, javascriptValue));
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
    bool succ = success;

    androidGetApp()->main.runOnMainThread(
            [succ, path]() { android_statics.imgDialogCallback(succ, path); });
}

JNIEXPORT void JNICALL Java_com_elf_MZGLActivity_importFileComplete(JNIEnv *env,
                                                                    jobject thiz,
                                                                    jboolean success,
                                                                    jstring filePath) {
    string path = jstringToString(env, filePath);
    bool succ = success;

    androidGetApp()->main.runOnMainThread(
            [succ, path]() { android_statics.fileDialogCallback(path, succ); });
}

JNIEXPORT void JNICALL Java_com_elf_MZGLActivity_textboxDialogComplete(JNIEnv *jni,
                                                                       jobject,
                                                                       jboolean success,
                                                                       jstring text) {
    string s = jstringToString(jni, text);
    bool succ = success;
    androidGetApp()->main.runOnMainThread(
            [s, succ]() { android_statics.completionCallback(s, succ); });
}

#ifdef __cplusplus
}
#endif
