#pragma once

// The smallest useful mzgl ::Plugin: a stereo gain with one automatable
// parameter. This ONE class is what every plugin format wraps - the VST3
// (GainVST3.cpp), the AUv2 (GainAUv2.cpp) and the AUv3 (mzgl's MZGLEffectAU,
// via instantiatePlugin() in entryPoints.cpp).
//
// Threading: process() and midiReceivedAtTime() run on the audio thread; the
// parameter values are atomics inside PluginParameter, so reading them there is
// fine. Everything else (serialize/deserialize, init) is main thread.

#include "Plugin.h"

#include <cmath>
#include <cstring>

class GainPlugin : public Plugin {
public:
	enum Param { kGain = 0 };

	GainPlugin() {
		setIsInstrument(false);
		// Parameters are registered in a fixed order; their index is the host
		// parameter ID for all three formats, so never reorder them - add at the end.
		addFloatParameter("Gain", 0.f, -24.f, 24.f, "dB");
	}

	// Serializable - the identifier names the user-preset file extension
	// (".gainpreset") and the state chunk. Keep it stable.
	std::string getIdentifier() override { return "gain"; }

	void serialize(std::vector<uint8_t> &outData) override {
		// Format: magic + one float per parameter, in parameter order.
		outData.clear();
		outData.insert(outData.end(), kMagic, kMagic + 4);
		for (size_t i = 0; i < getNumParams(); ++i) {
			const float v = getParam(static_cast<unsigned int>(i))->get();
			const auto *p = reinterpret_cast<const uint8_t *>(&v);
			outData.insert(outData.end(), p, p + sizeof(float));
		}
	}

	void deserialize(const std::vector<uint8_t> &data) override {
		if (data.size() < 4 || std::memcmp(data.data(), kMagic, 4) != 0) return;
		size_t offset = 4;
		for (size_t i = 0; i < getNumParams() && offset + sizeof(float) <= data.size(); ++i) {
			float v;
			std::memcpy(&v, data.data() + offset, sizeof(float));
			offset += sizeof(float);
			// Route through the host so its parameter cache / automation lane
			// follow the restored state.
			updateHostParameter(static_cast<unsigned int>(i), v);
		}
	}

	// Plugin
	void init(int numInputs, int numOutputs) override {}

	// ins/outs are interleaved stereo bus buffers.
	void process(FloatBuffer *ins, FloatBuffer *outs, int channelsPerBus) override {
		FloatBuffer &in	 = ins[0];
		FloatBuffer &out = outs[0];
		if (out.size() != in.size()) out.resize(in.size());

		const float gainDb = getParam(kGain)->get();
		const float gain   = std::pow(10.f, gainDb / 20.f);
		for (size_t i = 0; i < in.size(); ++i) {
			out[i] = in[i] * gain;
		}
	}

private:
	static constexpr uint8_t kMagic[4] = {'G', 'A', 'I', 'N'};
};
