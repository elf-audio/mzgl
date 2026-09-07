#include "MzglVST3SingleComponent.h"

#include "Plugin.h"
#include "PluginParameter.h"
#include "MidiMessage.h"
#include "MzglVST3MidiCC.h"

#include "pluginterfaces/base/ibstream.h"
#include "pluginterfaces/base/ustring.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "pluginterfaces/vst/ivstprocesscontext.h"
#include "base/source/fstreamer.h"

#include <algorithm>
#include <cmath>
#include <cstring>

using namespace Steinberg;
using namespace Steinberg::Vst;

namespace mzglvst {

MzglVST3SingleComponent::MzglVST3SingleComponent()	= default;
MzglVST3SingleComponent::~MzglVST3SingleComponent() = default;

float MzglVST3SingleComponent::normalize(unsigned int paramIndex, float value) const {
	auto p			 = plugin->getParam(paramIndex);
	const float span = p->to - p->from;
	if (std::abs(span) < 1e-9f) return 0.f;
	return std::clamp((value - p->from) / span, 0.f, 1.f);
}

float MzglVST3SingleComponent::denormalize(unsigned int paramIndex, float normalized) const {
	auto p			 = plugin->getParam(paramIndex);
	const float span = p->to - p->from;
	float value		 = p->from + std::clamp(normalized, 0.f, 1.f) * span;
	if (p->type == PluginParameter::Type::Int || p->type == PluginParameter::Type::Indexed) {
		value = std::round(value);
	}
	return value;
}

void MzglVST3SingleComponent::registerParameters() {
	const size_t numParams = plugin->getNumParams();
	for (size_t i = 0; i < numParams; ++i) {
		auto p = plugin->getParam(static_cast<unsigned int>(i));

		String128 title;
		UString(title, 128).fromAscii(p->name.c_str());
		String128 units;
		UString(units, 128).fromAscii(p->unit.c_str());

		int32 stepCount = 0;
		if (p->type == PluginParameter::Type::Indexed) {
			stepCount = p->options.empty() ? 1 : static_cast<int32>(p->options.size() - 1);
		} else if (p->type == PluginParameter::Type::Int) {
			stepCount = std::max<int32>(0, static_cast<int32>(std::round(p->to - p->from)));
		}

		const ParamValue defaultNormalized = normalize(static_cast<unsigned int>(i), p->defaultValue);
		auto *vst3Param					   = new Steinberg::Vst::Parameter(
			title, static_cast<ParamID>(i), units, defaultNormalized, stepCount, ParameterInfo::kCanAutomate);
		parameters.addParameter(vst3Param);

		EditControllerEx1::setParamNormalized(static_cast<ParamID>(i),
											  normalize(static_cast<unsigned int>(i), p->get()));
	}

	if (config.midiCCProxies) {
		midicc::addProxyParameters(parameters);
	}
}

tresult PLUGIN_API MzglVST3SingleComponent::initialize(FUnknown *context) {
	tresult result = SingleComponentEffect::initialize(context);
	if (result != kResultOk) return result;

	config = getConfig();
	if (config.needsTransport) {
		processContextRequirements.needTempo();
		processContextRequirements.needTransportState();
		processContextRequirements.needProjectTimeMusic();
	}

	plugin = createPlugin();
	if (!plugin) return kResultFalse;

	if (config.audioInput) {
		addAudioInput(STR16("Stereo In"), SpeakerArr::kStereo);
	}
	addAudioOutput(STR16("Stereo Out"), SpeakerArr::kStereo);
	if (config.midiInput) {
		addEventInput(STR16("MIDI In"), 1);
	}

	numInternalOutputBusses = std::max(1, plugin->getNumOutputBusses());

	registerParameters();

	plugin->sendUpdatedParameterToHost = [this](unsigned int i, float value) {
		if (!plugin || i >= plugin->getNumParams()) return;
		const ParamValue n = normalize(i, value);
		beginEdit(static_cast<ParamID>(i));
		performEdit(static_cast<ParamID>(i), n);
		endEdit(static_cast<ParamID>(i));
		EditControllerEx1::setParamNormalized(static_cast<ParamID>(i), n);
	};

	interleavedIn.reserve(8192 * 2);
	return kResultOk;
}

tresult PLUGIN_API MzglVST3SingleComponent::terminate() {
	if (plugin) {
		plugin->sendUpdatedParameterToHost = nullptr;
	}
	plugin.reset();
	return SingleComponentEffect::terminate();
}

tresult PLUGIN_API MzglVST3SingleComponent::setActive(TBool state) {
	if (plugin) {
		onActivate(state != 0);
		if (state) {
			plugin->init(2, 2);
		}
	}
	return SingleComponentEffect::setActive(state);
}

tresult PLUGIN_API MzglVST3SingleComponent::setupProcessing(ProcessSetup &setup) {
	tresult result = SingleComponentEffect::setupProcessing(setup);
	if (result != kResultOk) return result;
	if (plugin) {
		plugin->setSampleRate(setup.sampleRate);
	}
	return kResultOk;
}

tresult PLUGIN_API MzglVST3SingleComponent::canProcessSampleSize(int32 symbolicSampleSize) {
	return (symbolicSampleSize == kSample32) ? kResultTrue : kResultFalse;
}

tresult PLUGIN_API MzglVST3SingleComponent::setBusArrangements(SpeakerArrangement *inputs,
															   int32 numIns,
															   SpeakerArrangement *outputs,
															   int32 numOuts) {
	const int32 wantedIns = config.audioInput ? 1 : 0;
	if (numIns == wantedIns && numOuts == 1 && SpeakerArr::getChannelCount(outputs[0]) == 2
		&& (numIns == 0 || SpeakerArr::getChannelCount(inputs[0]) == 2)) {
		return SingleComponentEffect::setBusArrangements(inputs, numIns, outputs, numOuts);
	}
	return kResultFalse;
}

void MzglVST3SingleComponent::handleParameterChanges(IParameterChanges *changes) {
	if (!changes || !plugin) return;
	const int32 numParamsChanged = changes->getParameterCount();
	for (int32 i = 0; i < numParamsChanged; ++i) {
		IParamValueQueue *queue = changes->getParameterData(i);
		if (!queue) continue;
		const int32 numPoints = queue->getPointCount();
		if (numPoints <= 0) continue;
		const ParamID id = queue->getParameterId();

		if (config.midiCCProxies && midicc::isProxyParamId(id)) {
			for (int32 point = 0; point < numPoints; ++point) {
				int32 sampleOffset;
				ParamValue value;
				if (queue->getPoint(point, sampleOffset, value) != kResultOk) continue;
				plugin->midiReceivedAtTime(midicc::proxyParamToMidiMessage(id, value),
										   static_cast<uint32_t>(std::max<int32>(0, sampleOffset)));
			}
			continue;
		}

		int32 sampleOffset;
		ParamValue value;
		if (queue->getPoint(numPoints - 1, sampleOffset, value) != kResultOk) continue;
		if (id >= plugin->getNumParams()) continue;
		plugin->hostUpdatedParameter(id, denormalize(id, static_cast<float>(value)));
	}
}

void MzglVST3SingleComponent::handleEvents(IEventList *events) {
	if (!events || !plugin || !config.midiInput) return;
	const int32 numEvents = events->getEventCount();
	for (int32 i = 0; i < numEvents; ++i) {
		Event e;
		if (events->getEvent(i, e) != kResultOk) continue;
		const uint32_t delay = static_cast<uint32_t>(std::max<int32>(0, e.sampleOffset));
		switch (e.type) {
			case Event::kNoteOnEvent: {
				int velocity = std::clamp(static_cast<int>(std::round(e.noteOn.velocity * 127.f)), 1, 127);
				plugin->midiReceivedAtTime(MidiMessage::noteOn(e.noteOn.channel + 1, e.noteOn.pitch, velocity),
										   delay);
				break;
			}
			case Event::kNoteOffEvent:
				plugin->midiReceivedAtTime(MidiMessage::noteOff(e.noteOff.channel + 1, e.noteOff.pitch), delay);
				break;
			case Event::kPolyPressureEvent: {
				const auto &p = e.polyPressure;
				plugin->midiReceivedAtTime(
					MidiMessage {static_cast<uint8_t>(MidiMessageConstants::MIDI_POLY_AFTERTOUCH | (p.channel & 15)),
								 static_cast<uint8_t>(p.pitch & 127),
								 midicc::to7Bit(p.pressure)},
					delay);
				break;
			}
			default:
				break;
		}
	}
}

void MzglVST3SingleComponent::applyTransport(ProcessContext *ctx) {
	if (!ctx || !plugin) return;
	plugin->setHostIsPlaying((ctx->state & ProcessContext::kPlaying) != 0);
	if (ctx->state & ProcessContext::kTempoValid) plugin->bpm = ctx->tempo;
	if (ctx->state & ProcessContext::kProjectTimeMusicValid) plugin->beatPosition = ctx->projectTimeMusic;
}

tresult PLUGIN_API MzglVST3SingleComponent::process(ProcessData &data) {
	handleParameterChanges(data.inputParameterChanges);
	applyTransport(data.processContext);
	handleEvents(data.inputEvents);

	if (!plugin || data.numSamples <= 0) return kResultOk;

	const int32 numSamples		 = data.numSamples;
	const int32 channelsPerBus	 = 2;
	const size_t interleavedSize = static_cast<size_t>(numSamples) * channelsPerBus;

	interleavedIn.resize(interleavedSize);
	if (config.audioInput && data.numInputs >= 1 && data.inputs[0].numChannels >= 2) {
		const float *left  = data.inputs[0].channelBuffers32[0];
		const float *right = data.inputs[0].channelBuffers32[1];
		for (int32 i = 0; i < numSamples; ++i) {
			interleavedIn[i * 2]	 = left[i];
			interleavedIn[i * 2 + 1] = right[i];
		}
	} else {
		interleavedIn.zeros();
	}

	if (static_cast<int32>(interleavedOuts.size()) != numInternalOutputBusses) {
		interleavedOuts.resize(numInternalOutputBusses);
	}
	for (auto &o: interleavedOuts) {
		o.resize(interleavedSize);
	}

	plugin->process(&interleavedIn, interleavedOuts.data(), channelsPerBus);

	if (data.numOutputs >= 1 && data.outputs[0].numChannels >= 2) {
		float *left			   = data.outputs[0].channelBuffers32[0];
		float *right		   = data.outputs[0].channelBuffers32[1];
		const FloatBuffer &src = interleavedOuts[0];
		for (int32 i = 0; i < numSamples; ++i) {
			left[i]	 = src[i * 2];
			right[i] = src[i * 2 + 1];
		}
		data.outputs[0].silenceFlags = 0;
	}
	return kResultOk;
}

tresult PLUGIN_API MzglVST3SingleComponent::setState(IBStream *state) {
	if (!state || !plugin) return kResultFalse;
	IBStreamer streamer(state, kLittleEndian);
	int32 size = 0;
	if (!streamer.readInt32(size)) return kResultFalse;
	if (size <= 0) return kResultOk;
	std::vector<uint8_t> blob(static_cast<size_t>(size));
	if (streamer.readRaw(blob.data(), size) != size) return kResultFalse;
	plugin->deserialize(blob);
	return kResultOk;
}

tresult PLUGIN_API MzglVST3SingleComponent::getState(IBStream *state) {
	if (!state || !plugin) return kResultFalse;
	std::vector<uint8_t> blob;
	plugin->serialize(blob);
	IBStreamer streamer(state, kLittleEndian);
	const int32 size = static_cast<int32>(blob.size());
	if (!streamer.writeInt32(size)) return kResultFalse;
	if (size > 0 && streamer.writeRaw(blob.data(), size) != size) return kResultFalse;
	return kResultOk;
}

tresult PLUGIN_API MzglVST3SingleComponent::getMidiControllerAssignment(int32 busIndex,
																		int16 channel,
																		CtrlNumber midiControllerNumber,
																		ParamID &id) {
	if (!config.midiCCProxies || busIndex != 0) return kResultFalse;
	return midicc::lookupAssignment(channel, midiControllerNumber, id) ? kResultTrue : kResultFalse;
}

tresult PLUGIN_API MzglVST3SingleComponent::queryInterface(const TUID iid, void **obj) {
	if (config.midiCCProxies) {
		QUERY_INTERFACE(iid, obj, IMidiMapping::iid, IMidiMapping)
	}
	return SingleComponentEffect::queryInterface(iid, obj);
}

tresult PLUGIN_API MzglVST3SingleComponent::setParamNormalized(ParamID tag, ParamValue value) {
	const tresult result = EditControllerEx1::setParamNormalized(tag, value);
	if (plugin && tag < plugin->getNumParams()) {
		plugin->hostUpdatedParameter(tag, denormalize(tag, static_cast<float>(value)));
	}
	return result;
}

tresult PLUGIN_API MzglVST3SingleComponent::getParamStringByValue(ParamID tag,
																  ParamValue valueNormalized,
																  String128 string) {
	if (plugin && tag < plugin->getNumParams()) {
		auto p = plugin->getParam(tag);
		char buf[64];
		const float v = denormalize(tag, static_cast<float>(valueNormalized));
		if (p->type == PluginParameter::Type::Indexed && !p->options.empty()) {
			const int idx = std::clamp(static_cast<int>(std::round(v)), 0, static_cast<int>(p->options.size()) - 1);
			snprintf(buf, sizeof(buf), "%s", p->options[static_cast<size_t>(idx)].c_str());
		} else if (p->type == PluginParameter::Type::Int) {
			snprintf(buf, sizeof(buf), "%d", static_cast<int>(std::round(v)));
		} else {
			snprintf(buf, sizeof(buf), "%.2f", v);
		}
		UString(string, 128).fromAscii(buf);
		return kResultTrue;
	}
	return EditControllerEx1::getParamStringByValue(tag, valueNormalized, string);
}

IPlugView *PLUGIN_API MzglVST3SingleComponent::createView(FIDString name) {
	if (name && strcmp(name, ViewType::kEditor) == 0) {
		return new MzglVST3View(this, config.view);
	}
	return nullptr;
}

} // namespace mzglvst
