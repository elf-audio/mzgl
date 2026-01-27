#include "MZGLEffectVST3.h"
#include "MZGLEffectVST3View.h"
#include "Plugin.h"
#include "PluginEditor.h"
#include "Midi.h"

#if defined(__clang__)
#	pragma clang diagnostic push
#	pragma clang diagnostic ignored "-Wnon-virtual-dtor"
#endif
#include "pluginterfaces/base/ibstream.h"
#include "pluginterfaces/base/funknown.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "base/source/fstreamer.h"
#if defined(__clang__)
#	pragma clang diagnostic pop
#endif

#include <cstring>
#include <algorithm>

using namespace Steinberg;
using namespace Steinberg::Vst;

MZGLEffectVST3::MZGLEffectVST3() {
}

MZGLEffectVST3::~MZGLEffectVST3() {
	plugin.reset();
}

tresult PLUGIN_API MZGLEffectVST3::initialize(FUnknown *context) {
	tresult result = SingleComponentEffect::initialize(context);
	if (result != kResultOk) {
		return result;
	}

	plugin = createPlugin();
	if (!plugin) {
		return kResultFalse;
	}

	addAudioInput(STR16("Stereo In"), SpeakerArr::kStereo);

	int numOutputBusses = plugin->getNumOutputBusses();
	if (numOutputBusses <= 1) {
		addAudioOutput(STR16("Stereo Out"), SpeakerArr::kStereo);
	} else {
		for (int i = 0; i < numOutputBusses; i++) {
			char16 name[32];
			snprintf(reinterpret_cast<char *>(name), sizeof(name), "Output %d", i + 1);
			addAudioOutput(name, SpeakerArr::kStereo);
		}
	}

	addEventInput(STR16("MIDI In"), 1);

	if (plugin->getNumMidiOuts() > 0) {
		addEventOutput(STR16("MIDI Out"), plugin->getNumMidiOuts());
	}

	setupParameters();

	plugin->sendUpdatedParameterToHost = [this](unsigned int paramId, float value) {
		if (paramId < plugin->getNumParams()) {
			auto param = plugin->getParam(paramId);
			if (param) {
				ParamValue normalized = (value - param->from) / (param->to - param->from);
				setParamNormalized(paramId, normalized);
			}
		}
	};

	return kResultOk;
}

void MZGLEffectVST3::setupParameters() {
	if (!plugin) return;

	size_t numParams = plugin->getNumParams();
	for (size_t i = 0; i < numParams; i++) {
		auto param = plugin->getParam(static_cast<unsigned int>(i));
		if (!param) continue;

		String128 paramName;
		std::string name = param->name;
		for (size_t j = 0; j < name.length() && j < 127; j++) {
			paramName[j] = static_cast<char16>(name[j]);
		}
		paramName[std::min(name.length(), size_t(127))] = 0;

		ParamValue defaultNormalized = (param->defaultValue - param->from) / (param->to - param->from);

		parameters.addParameter(
			paramName, nullptr, 0, defaultNormalized, ParameterInfo::kCanAutomate, static_cast<ParamID>(i));
	}
}

tresult PLUGIN_API MZGLEffectVST3::terminate() {
	plugin.reset();
	return SingleComponentEffect::terminate();
}

tresult PLUGIN_API MZGLEffectVST3::setBusArrangements(SpeakerArrangement *inputs,
													  int32 numIns,
													  SpeakerArrangement *outputs,
													  int32 numOuts) {
	if (numIns >= 1 && numOuts >= 1) {
		if (inputs[0] == SpeakerArr::kStereo && outputs[0] == SpeakerArr::kStereo) {
			return SingleComponentEffect::setBusArrangements(inputs, numIns, outputs, numOuts);
		}
	}
	return kResultFalse;
}

tresult PLUGIN_API MZGLEffectVST3::setActive(TBool state) {
	if (state) {
		size_t bufferSize = static_cast<size_t>(maxBlockSize) * 2;
		inputBuffer.resize(bufferSize, 0.0f);

		int numOutputBusses = plugin ? plugin->getNumOutputBusses() : 1;
		if (numOutputBusses < 1) numOutputBusses = 1;
		outputBuffers.resize(numOutputBusses);
		for (auto &buf: outputBuffers) {
			buf.resize(bufferSize, 0.0f);
		}

		if (plugin) {
			plugin->init(2, 2);
		}
	} else {
		inputBuffer.clear();
		outputBuffers.clear();
	}

	isProcessing = state != 0;
	return SingleComponentEffect::setActive(state);
}

tresult PLUGIN_API MZGLEffectVST3::setupProcessing(ProcessSetup &setup) {
	sampleRate	 = setup.sampleRate;
	maxBlockSize = setup.maxSamplesPerBlock;

	if (plugin) {
		plugin->setSampleRate(sampleRate);
	}

	return SingleComponentEffect::setupProcessing(setup);
}

tresult PLUGIN_API MZGLEffectVST3::canProcessSampleSize(int32 symbolicSampleSize) {
	if (symbolicSampleSize == kSample32) {
		return kResultTrue;
	}
	return kResultFalse;
}

tresult PLUGIN_API MZGLEffectVST3::process(ProcessData &data) {
	if (!plugin) {
		return kResultOk;
	}

	plugin->hasStarted = true;

	if (data.processContext) {
		updateTransportInfo(data.processContext);
	}

	if (data.inputParameterChanges) {
		processParameterChanges(data.inputParameterChanges);
	}

	if (data.inputEvents) {
		processEvents(data.inputEvents);
	}

	if (data.numSamples > 0 && data.numOutputs > 0) {
		int32 numSamples = data.numSamples;
		int numChannels	 = 2;

		size_t requiredSize = static_cast<size_t>(numSamples) * numChannels;

		if (inputBuffer.size() != requiredSize) {
			inputBuffer.resize(requiredSize, 0.0f);
		}

		int numOutputBusses = plugin->getNumOutputBusses();
		if (numOutputBusses < 1) numOutputBusses = 1;
		if (outputBuffers.size() != static_cast<size_t>(numOutputBusses)) {
			outputBuffers.resize(numOutputBusses);
		}
		for (auto &buf: outputBuffers) {
			if (buf.size() != requiredSize) {
				buf.resize(requiredSize, 0.0f);
			}
		}

		bool hasInput = false;
		if (data.numInputs > 0 && data.inputs[0].numChannels >= 2) {
			float *inL = data.inputs[0].channelBuffers32[0];
			float *inR = data.inputs[0].channelBuffers32[1];

			if (inL != nullptr && inR != nullptr) {
				for (int32 i = 0; i < numSamples; i++) {
					inputBuffer[i * 2]	   = inL[i];
					inputBuffer[i * 2 + 1] = inR[i];
				}
				hasInput = true;
			}
		}

		if (!hasInput) {
			std::fill(inputBuffer.begin(), inputBuffer.begin() + requiredSize, 0.0f);
		}

		plugin->process(&inputBuffer, outputBuffers.data(), numChannels);

		for (int32 busIndex = 0; busIndex < data.numOutputs; busIndex++) {
			if (busIndex >= static_cast<int32>(outputBuffers.size())) {
				if (data.outputs[busIndex].numChannels >= 2) {
					float *outL = data.outputs[busIndex].channelBuffers32[0];
					float *outR = data.outputs[busIndex].channelBuffers32[1];
					if (outL) memset(outL, 0, sizeof(float) * numSamples);
					if (outR) memset(outR, 0, sizeof(float) * numSamples);
				}
				continue;
			}

			if (data.outputs[busIndex].numChannels >= 2) {
				float *outL = data.outputs[busIndex].channelBuffers32[0];
				float *outR = data.outputs[busIndex].channelBuffers32[1];

				if (outL != nullptr && outR != nullptr) {
					for (int32 i = 0; i < numSamples; i++) {
						outL[i] = outputBuffers[busIndex][i * 2];
						outR[i] = outputBuffers[busIndex][i * 2 + 1];
					}
				}
			}
		}
	}

	processMidiOutput(data);

	return kResultOk;
}

void MZGLEffectVST3::processEvents(IEventList *events) {
	if (!events || !plugin) return;

	int32 numEvents = events->getEventCount();
	for (int32 i = 0; i < numEvents; i++) {
		Event event;
		if (events->getEvent(i, event) == kResultOk) {
			switch (event.type) {
				case Event::kNoteOnEvent: {
					MidiMessage msg = MidiMessage::noteOn(event.noteOn.channel,
														  event.noteOn.pitch,
														  static_cast<int>(event.noteOn.velocity * 127.0f));
					plugin->midiReceivedAtTime(msg, event.sampleOffset);
					break;
				}
				case Event::kNoteOffEvent: {
					MidiMessage msg = MidiMessage::noteOff(event.noteOff.channel, event.noteOff.pitch);
					plugin->midiReceivedAtTime(msg, event.sampleOffset);
					break;
				}
				case Event::kDataEvent: {
					if (event.data.type == DataEvent::kMidiSysEx) {
						MidiMessage msg(event.data.bytes, event.data.size);
						plugin->midiReceivedAtTime(msg, event.sampleOffset);
					}
					break;
				}
				default: break;
			}
		}
	}
}

void MZGLEffectVST3::processParameterChanges(IParameterChanges *paramChanges) {
	if (!paramChanges || !plugin) return;

	int32 numParamsChanged = paramChanges->getParameterCount();
	for (int32 i = 0; i < numParamsChanged; i++) {
		IParamValueQueue *paramQueue = paramChanges->getParameterData(i);
		if (paramQueue) {
			ParamID paramId = paramQueue->getParameterId();
			int32 numPoints = paramQueue->getPointCount();

			if (numPoints > 0 && paramId < plugin->getNumParams()) {
				int32 sampleOffset;
				ParamValue value;
				if (paramQueue->getPoint(numPoints - 1, sampleOffset, value) == kResultOk) {
					auto param = plugin->getParam(static_cast<unsigned int>(paramId));
					if (param) {
						float denormalized = static_cast<float>(param->from + value * (param->to - param->from));
						plugin->hostUpdatedParameter(static_cast<unsigned int>(paramId), denormalized);
					}
				}
			}
		}
	}
}

void MZGLEffectVST3::updateTransportInfo(ProcessContext *context) {
	if (!context || !plugin) return;

	if (context->state & ProcessContext::kTempoValid) {
		plugin->bpm = context->tempo;
	}

	if (context->state & ProcessContext::kProjectTimeMusicValid) {
		plugin->beatPosition = context->projectTimeMusic;
	}

	bool isPlaying = (context->state & ProcessContext::kPlaying) != 0;
	if (lastHostIsPlaying != isPlaying) {
		plugin->setHostIsPlaying(isPlaying);
		lastHostIsPlaying = isPlaying;
	}
}

void MZGLEffectVST3::processMidiOutput(ProcessData &data) {
	if (!plugin || plugin->getNumMidiOuts() == 0) return;
	if (plugin->midiOutMessages.empty()) return;

	IEventList *outputEvents = data.outputEvents;
	if (!outputEvents) {
		plugin->midiOutMessages.clear();
		return;
	}

	for (const auto &m: plugin->midiOutMessages) {
		auto bytes = m.msg.getBytes();
		if (bytes.size() >= 3) {
			Event event		   = {};
			event.sampleOffset = m.delay;
			event.ppqPosition  = 0;
			event.flags		   = 0;

			uint8_t status	= bytes[0];
			uint8_t channel = status & 0x0F;
			uint8_t type	= status & 0xF0;

			if (type == 0x90 && bytes[2] > 0) {
				event.type			  = Event::kNoteOnEvent;
				event.noteOn.channel  = channel;
				event.noteOn.pitch	  = bytes[1];
				event.noteOn.velocity = static_cast<float>(bytes[2]) / 127.0f;
				event.noteOn.length	  = 0;
				event.noteOn.tuning	  = 0.0f;
				event.noteOn.noteId	  = -1;
				outputEvents->addEvent(event);
			} else if (type == 0x80 || (type == 0x90 && bytes[2] == 0)) {
				event.type			   = Event::kNoteOffEvent;
				event.noteOff.channel  = channel;
				event.noteOff.pitch	   = bytes[1];
				event.noteOff.velocity = static_cast<float>(bytes[2]) / 127.0f;
				event.noteOff.noteId   = -1;
				event.noteOff.tuning   = 0.0f;
				outputEvents->addEvent(event);
			}
		}
	}

	plugin->midiOutMessages.clear();
}

IPlugView *PLUGIN_API MZGLEffectVST3::createView(FIDString name) {
	if (FIDStringsEqual(name, ViewType::kEditor)) {
		return new MZGLEffectVST3View(this);
	}
	return nullptr;
}

tresult PLUGIN_API MZGLEffectVST3::setParamNormalized(ParamID tag, ParamValue value) {
	tresult result = SingleComponentEffect::setParamNormalized(tag, value);

	if (plugin && tag < plugin->getNumParams()) {
		auto param = plugin->getParam(static_cast<unsigned int>(tag));
		if (param) {
			float denormalized = static_cast<float>(param->from + value * (param->to - param->from));
			plugin->hostUpdatedParameter(static_cast<unsigned int>(tag), denormalized);
		}
	}

	return result;
}

ParamValue PLUGIN_API MZGLEffectVST3::getParamNormalized(ParamID tag) {
	if (plugin && tag < plugin->getNumParams()) {
		auto param = plugin->getParam(static_cast<unsigned int>(tag));
		if (param) {
			float value = param->get();
			return (value - param->from) / (param->to - param->from);
		}
	}
	return SingleComponentEffect::getParamNormalized(tag);
}

void MZGLEffectVST3::updateParameterFromUI(ParamID tag, ParamValue value) {
	setParamNormalized(tag, value);

	if (componentHandler) {
		componentHandler->beginEdit(tag);
		componentHandler->performEdit(tag, value);
		componentHandler->endEdit(tag);
	}
}

tresult PLUGIN_API MZGLEffectVST3::getState(IBStream *state) {
	if (!plugin || !state) return kResultFalse;

	std::vector<uint8_t> data;
	plugin->serialize(data);

	uint32 size = static_cast<uint32>(data.size());
	state->write(&size, sizeof(size));

	if (size > 0) {
		state->write(data.data(), size);
	}

	return kResultOk;
}

tresult PLUGIN_API MZGLEffectVST3::setState(IBStream *state) {
	if (!plugin || !state) return kResultFalse;

	uint32 size = 0;
	if (state->read(&size, sizeof(size)) != kResultOk) {
		return kResultFalse;
	}

	if (size > 0) {
		std::vector<uint8_t> data(size);
		if (state->read(data.data(), size) != kResultOk) {
			return kResultFalse;
		}

		plugin->deserialize(data);
	}

	return kResultOk;
}

tresult PLUGIN_API MZGLEffectVST3View::queryInterface(const TUID _iid, void **obj) {
	QUERY_INTERFACE(_iid, obj, FUnknown::iid, IPlugView)
	QUERY_INTERFACE(_iid, obj, IPlugView::iid, IPlugView)
	return FObject::queryInterface(_iid, obj);
}