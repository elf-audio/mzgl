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
	std::atomic<bool> inProcess {false};

	std::shared_ptr<AudioIODeviceType> type;
	std::shared_ptr<AudioIODevice> dev;
	JuceAudioSystem &sys;
	JuceImpl(JuceAudioSystem &sys)
		: type(createJuceIODeviceType())
		, sys(sys) {
		type->addListener(this);
		type->scanForDevices();
		printDevices();
	}

	void audioDeviceListChanged() override {
		printDevices();
		sys.deviceChanges.notify([](auto *l) { l->audioDeviceChanged(); });
	}

	void audioDeviceError(const String &errorMessage) override {
		Log::e() << "Audio device error: " << errorMessage;
	}

	std::vector<float> interleavedIns;
	std::vector<float> interleavedOuts;

	void audioDeviceIOCallbackWithContext(const float *const *inputChannelData,
										  int numInputChannels,
										  float *const *outputChannelData,
										  int numOutputChannels,
										  int numSamples,
										  const AudioIODeviceCallbackContext &context) override {
		inProcess.store(true);
		if (numInputChannels > 0) {
			interleave(interleavedIns, inputChannelData, numInputChannels, numSamples);
			if (sys.inputCallback) {
				sys.inputCallback(interleavedIns.data(), numSamples, numInputChannels);
			}
		}

		if (numOutputChannels > 0 && outputChannelData != nullptr) {
			interleavedOuts.resize(numOutputChannels * numSamples);
			if (sys.outputCallback) {
				sys.outputCallback(interleavedOuts.data(), numSamples, numOutputChannels);
			}
			deinterleave(interleavedOuts, outputChannelData, numOutputChannels, numSamples);
		}
		inProcess.store(false);
	}

	static void
		interleave(std::vector<float> &interleaved, const float *const *data, int numChans, int numFrames) {
		interleaved.resize(numChans * numFrames);
		// interleave the input channels
		for (int chan = 0; chan < numChans; chan++) {
			for (int j = 0; j < numFrames; j++) {
				interleaved[chan + j * numChans] = data[chan][j];
			}
		}
	}

	static void deinterleave(std::vector<float> &interleaved, float *const *data, int numChans, int numFrames) {
		for (int chan = 0; chan < numChans; chan++) {
			for (int j = 0; j < numFrames; j++) {
				data[chan][j] = interleaved[j * numChans + chan];
			}
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
	: impl(std::make_shared<JuceImpl>(*this)) {
}

String partialMatch(const StringArray &names, const std::string &term) {
	for (auto &name: names) {
		if (name.find(term) != std::string::npos) {
			return name;
		}
	}
	return "";
}

std::string defaultName(std::shared_ptr<AudioIODeviceType> type, bool isInput) {
	auto ins	 = type->getDeviceNames(isInput);
	auto inIndex = type->getDefaultDeviceIndex(isInput);

	if (inIndex >= 0 && inIndex < ins.size()) {
		return ins[inIndex];
	}
	return "";
}

void printOutFactoids(std::shared_ptr<AudioIODevice> dev) {
	Log::d() << "==========================";
	Log::d() << "Type: " << dev->getTypeName();
	Log::d() << "Name: " << dev->getName();
	auto srs			 = dev->getAvailableSampleRates();
	std::string srString = "";
	for (auto sr: srs) {
		srString += std::to_string(sr) + ", ";
	}
	Log::d() << "Sample Rates: " << srString;

	auto bss			 = dev->getAvailableBufferSizes();
	std::string bsString = "";
	for (auto bs: bss) {
		bsString += std::to_string(bs) + ", ";
	}
	Log::d() << "Buffer Sizes: " << bsString;
	auto inChNames		   = dev->getInputChannelNames();
	auto outChNames		   = dev->getOutputChannelNames();
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
}

void JuceAudioSystem::setup(int numInChans, int numOutChans) {
	currInputName = "";

	this->numInChannels	 = numInChans;
	this->numOutChannels = numOutChans;
	if (numInChans > 0) {
		currInputName = defaultName(impl->type, true);
	}

	currOutputName = defaultName(impl->type, false);

	startCurrConfig();
}
bool JuceAudioSystem::isInsideAudioCallback() {
	return impl->inProcess.load();
}
void JuceAudioSystem::startCurrConfig() {
	if (isRunning()) {
		stop();
		impl->dev->close();
	}
	impl->dev = std::shared_ptr<AudioIODevice>(impl->type->createDevice(currOutputName, currInputName));
	printOutFactoids(impl->dev);

	BigInteger inChannels;
	BigInteger outChannels;
	inChannels.setRange(0, numInChannels, true);
	outChannels.setRange(0, numOutChannels, true);
	auto err = impl->dev->open(inChannels, outChannels, 48000, 256);
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
	std::vector<AudioPort> ports;
	auto names = impl->type->getDeviceNames(true);
	for (int i = 0; i < names.size(); i++) {
		AudioPort p;
		p.name = names[i];
		ports.emplace_back(p);
	}
	return ports;
}
std::vector<AudioPort> JuceAudioSystem::getOutputs() {
	std::vector<AudioPort> ports;

	auto names = impl->type->getDeviceNames(false);

	for (int i = 0; i < names.size(); i++) {
		AudioPort p;
		p.name = names[i];
		ports.emplace_back(p);
	}
	return ports;
}

float JuceAudioSystem::getSampleRate() const {
	if (!impl->dev) return 0;
	return impl->dev->getCurrentSampleRate();
}

int JuceAudioSystem::getBufferSize() const {
	if (!impl->dev) return 0;
	return impl->dev->getCurrentBufferSizeSamples();
}

bool JuceAudioSystem::setInput(const AudioPort &audioInput) {
	if (audioInput.name == currInputName) return true;
	currInputName = audioInput.name;
	startCurrConfig();
	return true;
}

bool JuceAudioSystem::setOutput(const AudioPort &audioOutput) {
	if (audioOutput.name == currOutputName) return true;
	currOutputName = audioOutput.name;
	startCurrConfig();
	return true;
}

AudioPort JuceAudioSystem::getInput() {
	if (impl->dev == nullptr) return {};
	AudioPort p;
	p.name = currInputName;
	return p;
}

AudioPort JuceAudioSystem::getOutput() {
	if (impl->dev == nullptr) return {};
	AudioPort p;
	p.name = currOutputName;
	return p;
}