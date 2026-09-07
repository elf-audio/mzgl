#pragma once

#include "pluginterfaces/vst/vsttypes.h"
#include "pluginterfaces/vst/ivstmidicontrollers.h"
#include "public.sdk/source/vst/vsteditcontroller.h"
#include "pluginterfaces/base/ustring.h"
#include "MidiMessage.h"

#include <algorithm>
#include <cmath>
#include <string>

namespace mzglvst::midicc {

/**
 * VST3 never delivers MIDI CCs (or pitch bend / channel aftertouch) as
 * events. Instead the host asks the controller, via IMidiMapping, for a
 * parameter to stand in for each (channel, controller) pair and then sends
 * controller movements as ordinary parameter changes.
 *
 * These helpers give every MIDI channel a block of proxy parameters - the
 * 128 CCs plus separate pitch bend and aftertouch lanes - and translate
 * changes to them back into MidiMessages for the engine.
 *
 * The proxy IDs live at a fixed base well above any real parameter so the
 * layout is stable regardless of how many parameters the plugin exposes.
 */
constexpr Steinberg::Vst::ParamID kParamBase = 0x40000000;
constexpr int kNumChannels					 = 16;
constexpr int kNumCCs						 = 128;
constexpr int kNumProxyParams				 = kNumChannels * (kNumCCs + 2);

// Lane layout: [ 16 x 128 CCs ][ 16 pitch bends ][ 16 aftertouches ]
constexpr Steinberg::Vst::ParamID ccParamId(int channel, int cc) {
	return kParamBase + static_cast<Steinberg::Vst::ParamID>(channel * kNumCCs + cc);
}

constexpr Steinberg::Vst::ParamID pitchBendParamId(int channel) {
	return kParamBase + static_cast<Steinberg::Vst::ParamID>(kNumChannels * kNumCCs + channel);
}

constexpr Steinberg::Vst::ParamID aftertouchParamId(int channel) {
	return kParamBase + static_cast<Steinberg::Vst::ParamID>(kNumChannels * (kNumCCs + 1) + channel);
}

constexpr bool isProxyParamId(Steinberg::Vst::ParamID id) {
	return id >= kParamBase && id < kParamBase + kNumProxyParams;
}

/**
 * Resolve the host's (channel, controller) query to a proxy ParamID.
 * kAfterTouch and kPitchBend arrive as pseudo controller numbers 128/129.
 */
inline bool lookupAssignment(Steinberg::int16 channel,
							 Steinberg::Vst::CtrlNumber controller,
							 Steinberg::Vst::ParamID &outId) {
	if (channel < 0 || channel >= kNumChannels) return false;

	if (controller >= 0 && controller < kNumCCs) {
		outId = ccParamId(channel, controller);
		return true;
	}
	if (controller == Steinberg::Vst::kPitchBend) {
		outId = pitchBendParamId(channel);
		return true;
	}
	if (controller == Steinberg::Vst::kAfterTouch) {
		outId = aftertouchParamId(channel);
		return true;
	}
	return false;
}

inline uint8_t to7Bit(double normalized) {
	return static_cast<uint8_t>(std::clamp(static_cast<int>(std::round(normalized * 127.0)), 0, 127));
}

/** Translate a change to a proxy parameter back into a MIDI message. */
inline MidiMessage proxyParamToMidiMessage(Steinberg::Vst::ParamID id, double normalized) {
	const int index = static_cast<int>(id - kParamBase);

	if (index < kNumChannels * kNumCCs) {
		const int channel = index / kNumCCs;
		const int cc	  = index % kNumCCs;
		// MidiMessage factories take 1-based channels.
		return MidiMessage::cc(channel + 1, cc, to7Bit(normalized));
	}

	if (index < kNumChannels * (kNumCCs + 1)) {
		const int channel = index - kNumChannels * kNumCCs;
		const int bend	  = std::clamp(static_cast<int>(std::round(normalized * 16383.0)), 0, 16383);
		return MidiMessage(static_cast<uint8_t>(MidiMessageConstants::MIDI_PITCH_BEND | channel),
						   static_cast<uint8_t>(bend & 0x7F),
						   static_cast<uint8_t>((bend >> 7) & 0x7F));
	}

	const int channel = index - kNumChannels * (kNumCCs + 1);
	return MidiMessage(static_cast<uint8_t>(MidiMessageConstants::MIDI_AFTERTOUCH | channel),
					   to7Bit(normalized));
}

/**
 * Register all the proxy parameters with a controller's parameter list.
 * They're deliberately not automatable - they exist purely as the host's
 * routing target for hardware MIDI, so they shouldn't clutter automation
 * choosers or get saved into presets.
 */
inline void addProxyParameters(Steinberg::Vst::ParameterContainer &parameters) {
	using namespace Steinberg;
	using namespace Steinberg::Vst;

	auto addProxy = [&parameters](const std::string &name, ParamID id, ParamValue defaultNormalized) {
		String128 title;
		UString(title, 128).fromAscii(name.c_str());
		parameters.addParameter(
			new Steinberg::Vst::Parameter(title, id, nullptr, defaultNormalized, 0, ParameterInfo::kNoFlags));
	};

	for (int channel = 0; channel < kNumChannels; ++channel) {
		const std::string chSuffix = " Ch " + std::to_string(channel + 1);
		for (int cc = 0; cc < kNumCCs; ++cc) {
			addProxy("MIDI CC " + std::to_string(cc) + chSuffix, ccParamId(channel, cc), 0.0);
		}
	}
	for (int channel = 0; channel < kNumChannels; ++channel) {
		addProxy("MIDI Pitch Bend Ch " + std::to_string(channel + 1), pitchBendParamId(channel), 0.5);
	}
	for (int channel = 0; channel < kNumChannels; ++channel) {
		addProxy("MIDI Aftertouch Ch " + std::to_string(channel + 1), aftertouchParamId(channel), 0.0);
	}
}

} // namespace mzglvst::midicc
