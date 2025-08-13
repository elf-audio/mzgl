#pragma once

#include <memory>

struct CoreAudioState;

#include "AudioSystem.h"

class CoreAudioSystem : public _AudioSystem {
public:
	CoreAudioSystem();
	~CoreAudioSystem() override;

	void setup(int numInChannels, int numOutChannels) override;
	void start() override;
	void stop() override;

	[[nodiscard]] bool isRunning() override;
	[[nodiscard]] bool isInsideAudioCallback() override;

	void setVerbose(bool _verbose) override;

	void rescanPorts() override;
	std::vector<AudioPort> getInputs() override;
	std::vector<AudioPort> getOutputs() override;

	void setSampleRate(float sampleRate) override;
	void setBufferSize(int size) override;

	[[nodiscard]] bool setInput(const AudioPort &audioInput) override;
	[[nodiscard]] bool setOutput(const AudioPort &audioOutput) override;
	[[nodiscard]] AudioPort getInput() override;
	[[nodiscard]] AudioPort getOutput() override;

	[[nodiscard]] double getOutputLatency() override;

	[[nodiscard]] double getNanoSecondsAtBufferBegin() override;
	[[nodiscard]] double getHostTime();

	[[nodiscard]] CoreAudioState &getState();

private:
	static constexpr auto defaultNumberOfFrames = 512;
	static constexpr auto defaultSampleRate		= 48'000.0;

	[[nodiscard]] double getPreferredSampleRate() const;
	[[nodiscard]] uint32_t getPreferredNumberOfFrames() const;
	void updateDefaultIOPorts();
	void updateRunningParameters();

	void setupState(int numInChannels, int numOutChannels);
	void createAudioUnit();
	void connectOutput();
	void enableAudioIO();
	void setupStreamFormat();
	void setupAudioBuffers();
	void setupCallback();

	void restart();

	void printDeviceList() const;

	std::unique_ptr<CoreAudioState> state;

	std::optional<AudioPort> inputPort;
	std::optional<AudioPort> outputPort;
	std::vector<AudioPort> inputPorts;
	std::vector<AudioPort> outputPorts;

	bool verbose {false};
};