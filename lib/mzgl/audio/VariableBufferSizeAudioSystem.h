/**
 * For testing problems with variable buffer sizes
 * caused by bluetooth devices or other things.
 */
#include "AudioSystem.h"

class VariableBufferSizeAudioSystem : public _AudioSystem {
public:
	VariableBufferSizeAudioSystem()
		: _AudioSystem() {
		sampleRate = 48000.f;
	}
	void setSampleRate(float sampleRate) override { this->sampleRate = sampleRate; }
	void setup(int numInChannels, int numOutChannels) override {}
	void start() override {
		if (running.load()) return; // Prevent multiple starts
		running.store(true);
		audioThread = std::thread([this]() {
			std::random_device rd;
			std::mt19937 gen(rd());
			std::uniform_int_distribution<int> dist(2, 4099);

			while (running.load()) {
				int numSamples = dist(gen) * 2;
				std::vector<float> buffer(numSamples, 0.0f);
				outputCallback(buffer.data(), numSamples / 2, 2);
				inputCallback(buffer.data(), numSamples / 2, 2);

				std::this_thread::sleep_for(std::chrono::milliseconds(5));
			}
		});
	}
	std::thread audioThread;
	std::atomic<bool> running {false};
	void stop() override {
		running.store(false);
		if (audioThread.joinable()) {
			audioThread.join();
		}
	}
	[[nodiscard]] bool isRunning() override { return false; }
	[[nodiscard]] bool isInsideAudioCallback() override { return false; }

	void setVerbose(bool v) override {}

	std::vector<AudioPort> getInputs() override { return {}; }
	std::vector<AudioPort> getOutputs() override { return {}; }
};
