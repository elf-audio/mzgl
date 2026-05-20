//
//  ThreadedDummyAudioSystem.h
//  mzgl
//
//  A software-driven audio system: spawns a thread that fires the
//  output / input callbacks at buffer-duration intervals, instead of
//  relying on the OS audio driver. Use this for UI tests on headless
//  / RDP machines where PortAudio's callback isn't reliably pumped,
//  or anywhere you want deterministic audio progression without
//  actually playing sound out the speakers.
//

#pragma once

#include "AudioSystem.h"

#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

class ThreadedDummyAudioSystem : public _AudioSystem {
public:
	ThreadedDummyAudioSystem() {
		sampleRate = 48'000.f;
		bufferSize = 256;
	}

	~ThreadedDummyAudioSystem() override { stop(); }

	void setup(int numIn, int numOut) override {
		numInChannels  = numIn;
		numOutChannels = numOut;
	}

	void setSampleRate(float sr) override {
		const bool wasRunning = running.load();
		if (wasRunning) stop();
		sampleRate = sr;
		notifySampleRateChanged();
		if (wasRunning) start();
	}

	void setBufferSize(int size) override {
		const bool wasRunning = running.load();
		if (wasRunning) stop();
		bufferSize = size;
		bufferSizeChangedBySystem(size);
		if (wasRunning) start();
	}

	void start() override {
		if (running.exchange(true)) return;
		worker = std::thread([this]() { run(); });
	}

	void stop() override {
		if (!running.exchange(false)) return;
		if (worker.joinable()) worker.join();
	}

	[[nodiscard]] bool isRunning() override { return running.load(); }
	[[nodiscard]] bool isInsideAudioCallback() override { return insideCallback.load(); }

	std::vector<AudioPort> getInputs() override { return {makePort("Threaded Dummy In", numInChannels, 0, true, false)}; }
	std::vector<AudioPort> getOutputs() override { return {makePort("Threaded Dummy Out", 0, numOutChannels, false, true)}; }

private:
	static AudioPort makePort(const std::string &name, int ins, int outs, bool defIn, bool defOut) {
		AudioPort p;
		p.portId			= 0;
		p.numInChannels		= ins;
		p.numOutChannels	= outs;
		p.defaultSampleRate = 48'000;
		p.name				= name;
		p.isDefaultInput	= defIn;
		p.isDefaultOutput	= defOut;
		return p;
	}

	void run() {
		// Output buffer is what `outputCallback` writes into; input buffer is
		// what `inputCallback` reads from. We zero both each tick - the engine
		// fills output, and any fake-mic synthesis happens inside the engine's
		// own audioOut (see KoalaAudioEngine fakeMic path).
		std::vector<float> inBuf(static_cast<size_t>(bufferSize) * static_cast<size_t>(std::max(1, numInChannels)));
		std::vector<float> outBuf(static_cast<size_t>(bufferSize) * static_cast<size_t>(std::max(1, numOutChannels)));

		using clock		= std::chrono::steady_clock;
		auto bufferNs	= std::chrono::nanoseconds {static_cast<int64_t>(1'000'000'000.0 * bufferSize / sampleRate)};
		auto nextWakeAt = clock::now();

		while (running.load()) {
			std::fill(inBuf.begin(), inBuf.end(), 0.0f);
			std::fill(outBuf.begin(), outBuf.end(), 0.0f);

			insideCallback.store(true);
			if (numInChannels > 0) inputCallback(inBuf.data(), bufferSize, numInChannels);
			if (numOutChannels > 0) outputCallback(outBuf.data(), bufferSize, numOutChannels);
			insideCallback.store(false);

			nextWakeAt += bufferNs;
			const auto now = clock::now();
			if (nextWakeAt > now) {
				std::this_thread::sleep_until(nextWakeAt);
			} else {
				// fell behind - skip ahead instead of trying to catch up in a
				// tight loop (tests don't care about timing accuracy here).
				nextWakeAt = now + bufferNs;
			}
		}
	}

	std::atomic<bool> running {false};
	std::atomic<bool> insideCallback {false};
	std::thread worker;
};
