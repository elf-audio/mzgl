#pragma once

#if defined(__clang__)
#	pragma clang diagnostic push
#	pragma clang diagnostic ignored "-Wnon-virtual-dtor"
#endif
#include "public.sdk/source/vst/vstsinglecomponenteffect.h"
#include "pluginterfaces/vst/ivstevents.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "pluginterfaces/vst/ivstprocesscontext.h"
#include "base/source/fstreamer.h"
#if defined(__clang__)
#	pragma clang diagnostic pop
#endif

#include "util/FloatBuffer.h"

#include <memory>
#include <vector>
#include <functional>

class Plugin;
class PluginEditor;
class Graphics;

class MZGLEffectVST3View;

class MZGLEffectVST3 : public Steinberg::Vst::SingleComponentEffect {
public:
	MZGLEffectVST3();
	~MZGLEffectVST3() override;

	Steinberg::tresult PLUGIN_API initialize(Steinberg::FUnknown *context) override;
	Steinberg::tresult PLUGIN_API terminate() override;

	Steinberg::tresult PLUGIN_API setBusArrangements(Steinberg::Vst::SpeakerArrangement *inputs,
													 Steinberg::int32 numIns,
													 Steinberg::Vst::SpeakerArrangement *outputs,
													 Steinberg::int32 numOuts) override;

	Steinberg::tresult PLUGIN_API setActive(Steinberg::TBool state) override;
	Steinberg::tresult PLUGIN_API setupProcessing(Steinberg::Vst::ProcessSetup &setup) override;
	Steinberg::tresult PLUGIN_API process(Steinberg::Vst::ProcessData &data) override;

	Steinberg::tresult PLUGIN_API canProcessSampleSize(Steinberg::int32 symbolicSampleSize) override;

	Steinberg::IPlugView *PLUGIN_API createView(Steinberg::FIDString name) override;

	Steinberg::tresult PLUGIN_API setParamNormalized(Steinberg::Vst::ParamID tag,
													 Steinberg::Vst::ParamValue value) override;
	Steinberg::Vst::ParamValue PLUGIN_API getParamNormalized(Steinberg::Vst::ParamID tag) override;

	Steinberg::tresult PLUGIN_API getState(Steinberg::IBStream *state) override;
	Steinberg::tresult PLUGIN_API setState(Steinberg::IBStream *state) override;

	std::shared_ptr<Plugin> getPlugin() { return plugin; }

	void updateParameterFromUI(Steinberg::Vst::ParamID tag, Steinberg::Vst::ParamValue value);

protected:
	virtual std::shared_ptr<Plugin> createPlugin() = 0;

	std::shared_ptr<Plugin> plugin;

	FloatBuffer inputBuffer;
	std::vector<FloatBuffer> outputBuffers;

	double sampleRate			  = 44100.0;
	Steinberg::int32 maxBlockSize = 0;
	bool isProcessing			  = false;
	bool lastHostIsPlaying		  = false;

	void processEvents(Steinberg::Vst::IEventList *events);
	void processParameterChanges(Steinberg::Vst::IParameterChanges *paramChanges);
	void updateTransportInfo(Steinberg::Vst::ProcessContext *context);
	void processMidiOutput(Steinberg::Vst::ProcessData &data);

	void setupParameters();
};