//
// Created by Marek Bereza on 26/03/2024.
//

#include "JuceAudioSystem.h"
#include "mzgl_juce_CoreAudio.h"
#include "log.h"

using namespace juce;
class JuceImpl
	: public AudioIODeviceType::Listener
	, public AudioIODeviceCallback {
public:
	std::shared_ptr<AudioIODeviceType> type;
	std::shared_ptr<AudioIODevice> dev;
	JuceImpl()
		: type(createJuceIODeviceType()) {
		type->addListener(this);
		type->scanForDevices();
		printDevices();
	}

	void audioDeviceListChanged() override {
		Log::d() << "DEVICES CHANGED";
		printDevices();
	}

	double phase = 0;
	double freq	 = 440;
	void audioDeviceIOCallbackWithContext(const float *const *inputChannelData,
										  int numInputChannels,
										  float *const *outputChannelData,
										  int numOutputChannels,
										  int numSamples,
										  const AudioIODeviceCallbackContext &context) override {
		for (int i = 0; i < numSamples; i++) {
			outputChannelData[0][i] = outputChannelData[1][i] = sin(phase) * inputChannelData[0][i];
			phase += M_PI * 2.0 * freq / 48000.0;
		}
	}

	void printDevices() {
		auto ins  = type->getDeviceNames(true);
		auto outs = type->getDeviceNames(false);

		for (auto in: ins) {
			Log::d() << "Input: " << in;
		}

		for (auto out: outs) {
			Log::d() << "Output: " << out;
		}
		Log::d() << "Scanned\n";
	}
};

JuceAudioSystem::JuceAudioSystem()
	: impl(std::make_shared<JuceImpl>()) {
}

String partialMatch(const StringArray &names, const std::string &term) {
	for (auto &name: names) {
		if (name.find(term) != std::string::npos) {
			return name;
		}
	}
	return "";
}

void JuceAudioSystem::setup(int numInChannels, int numOutChannels) {
	auto ins  = impl->type->getDeviceNames(true);
	auto outs = impl->type->getDeviceNames(false);

	auto in	 = partialMatch(ins, "2i2");
	auto out = partialMatch(outs, "2i2");
	if (in == "" || out == "") {
		Log::d() << "Didn't find outputs";
		return;
	}

	Log::d() << "Found our outputs";
	impl->dev = std::shared_ptr<AudioIODevice>(impl->type->createDevice(out, in));
	Log::d() << "==========================";
	Log::d() << "Type: " << impl->dev->getTypeName();
	Log::d() << "Name: " << impl->dev->getName();
	auto srs			 = impl->dev->getAvailableSampleRates();
	std::string srString = "";
	for (auto sr: srs) {
		srString += std::to_string(sr) + ", ";
	}
	Log::d() << "Sample Rates: " << srString;

	auto bss			 = impl->dev->getAvailableBufferSizes();
	std::string bsString = "";
	for (auto bs: bss) {
		bsString += std::to_string(bs) + ", ";
	}
	Log::d() << "Buffer Sizes: " << bsString;
	auto inChNames		   = impl->dev->getInputChannelNames();
	auto outChNames		   = impl->dev->getOutputChannelNames();
	std::string inChString = "";
	for (auto ch: inChNames) {
		inChString += ch + ", ";
	}
	Log::d() << "Input Channels: " << inChString;
	std::string outChString = "";
	for (auto ch: outChNames) {
		outChString += ch + ", ";
	}
	Log::d() << "Output Channels: " << outChString;

	BigInteger inChannels;
	BigInteger outChannels;
	inChannels.setRange(0, numInChannels, true);
	outChannels.setRange(0, numOutChannels, true);
	auto err = impl->dev->open(inChannels, outChannels, 48000, 128);
	if (!err.empty()) {
		Log::e() << "Got error trying to start audio: " << err;
	}

	impl->dev->start(impl.get());
}

void JuceAudioSystem::start() {
	if (impl->dev == nullptr) return;
	impl->dev->start(impl.get());
}

void JuceAudioSystem::stop() {
	if (impl->dev == nullptr) return;
	impl->dev->stop();
}

bool JuceAudioSystem::isRunning() {
	return impl->dev != nullptr && impl->dev->isPlaying();
}

std::vector<AudioPort> JuceAudioSystem::getInputs() {
	return {};
}
std::vector<AudioPort> JuceAudioSystem::getOutputs() {
	return {};
}

float JuceAudioSystem::getSampleRate() const {
	if (!impl->dev) return 0;
	impl->dev->getCurrentSampleRate();
}

int JuceAudioSystem::getBufferSize() const {
	if (!impl->dev) return 0;
	impl->dev->getCurrentBufferSizeSamples();
}
