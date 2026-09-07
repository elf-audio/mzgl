#pragma once

// Generic single-component VST3 wrapper around an mzgl ::Plugin.
//
// One object is both the audio processor and the edit controller, sharing ONE
// ::Plugin instance with the editor (the same model the AUv3 and AUv2 wrappers
// use), so knob movements, host automation and state all act on one object.
// Subclass it, return your ::Plugin from createPlugin() and a Config from
// getConfig(); see mzgl/examples/05_Plugin.
//
// Subclasses provide the ::Plugin and a Config; everything else (busses,
// parameters, automation, state, MIDI, transport, the Cocoa view) is here.

#include "public.sdk/source/vst/vstsinglecomponenteffect.h"
#include "pluginterfaces/vst/ivstevents.h"
#include "pluginterfaces/vst/ivstmidicontrollers.h"

#include "MzglVST3View.h"
#include "FloatBuffer.h"

#include <memory>
#include <vector>

class Plugin;

namespace mzglvst {

struct SingleComponentConfig {
	bool audioInput	   = true; // effect (true) or instrument (false)
	bool midiInput	   = false; // add a MIDI event bus + forward notes/aftertouch
	bool midiCCProxies = false; // expose per-channel CC/pitch-bend/aftertouch proxy params (IMidiMapping)
	bool needsTransport = true; // ask the host for tempo/transport/project time
	ViewConfig view;
};

class MzglVST3SingleComponent
	: public Steinberg::Vst::SingleComponentEffect
	, public Steinberg::Vst::IMidiMapping
	, public MzglVST3PluginProvider {
public:
	MzglVST3SingleComponent();
	~MzglVST3SingleComponent() override;

	Steinberg::tresult PLUGIN_API initialize(Steinberg::FUnknown *context) SMTG_OVERRIDE;
	Steinberg::tresult PLUGIN_API terminate() SMTG_OVERRIDE;

	Steinberg::tresult PLUGIN_API setActive(Steinberg::TBool state) SMTG_OVERRIDE;
	Steinberg::tresult PLUGIN_API setupProcessing(Steinberg::Vst::ProcessSetup &setup) SMTG_OVERRIDE;
	Steinberg::tresult PLUGIN_API canProcessSampleSize(Steinberg::int32 symbolicSampleSize) SMTG_OVERRIDE;
	Steinberg::tresult PLUGIN_API setBusArrangements(Steinberg::Vst::SpeakerArrangement *inputs,
													 Steinberg::int32 numIns,
													 Steinberg::Vst::SpeakerArrangement *outputs,
													 Steinberg::int32 numOuts) SMTG_OVERRIDE;
	Steinberg::tresult PLUGIN_API process(Steinberg::Vst::ProcessData &data) SMTG_OVERRIDE;

	Steinberg::tresult PLUGIN_API setState(Steinberg::IBStream *state) SMTG_OVERRIDE;
	Steinberg::tresult PLUGIN_API getState(Steinberg::IBStream *state) SMTG_OVERRIDE;

	Steinberg::tresult PLUGIN_API setParamNormalized(Steinberg::Vst::ParamID tag,
													 Steinberg::Vst::ParamValue value) SMTG_OVERRIDE;
	Steinberg::tresult PLUGIN_API getParamStringByValue(Steinberg::Vst::ParamID tag,
														Steinberg::Vst::ParamValue valueNormalized,
														Steinberg::Vst::String128 string) SMTG_OVERRIDE;

	// IMidiMapping (only answered when Config::midiCCProxies)
	Steinberg::tresult PLUGIN_API getMidiControllerAssignment(Steinberg::int32 busIndex,
															  Steinberg::int16 channel,
															  Steinberg::Vst::CtrlNumber midiControllerNumber,
															  Steinberg::Vst::ParamID &id) SMTG_OVERRIDE;

	Steinberg::tresult PLUGIN_API queryInterface(const Steinberg::TUID iid, void **obj) SMTG_OVERRIDE;
	Steinberg::uint32 PLUGIN_API addRef() SMTG_OVERRIDE { return SingleComponentEffect::addRef(); }
	Steinberg::uint32 PLUGIN_API release() SMTG_OVERRIDE { return SingleComponentEffect::release(); }

	Steinberg::IPlugView *PLUGIN_API createView(Steinberg::FIDString name) SMTG_OVERRIDE;

	std::shared_ptr<::Plugin> getPlugin() const override { return plugin; }

protected:
	/** Create the shared ::Plugin (called from initialize()). */
	virtual std::shared_ptr<::Plugin> createPlugin() = 0;
	virtual SingleComponentConfig getConfig() const = 0;
	/** Hooks around setActive(); default does nothing. */
	virtual void onActivate(bool active) {}

private:
	void registerParameters();
	void handleParameterChanges(Steinberg::Vst::IParameterChanges *changes);
	void handleEvents(Steinberg::Vst::IEventList *events);
	void applyTransport(Steinberg::Vst::ProcessContext *ctx);

	float denormalize(unsigned int paramIndex, float normalized) const;
	float normalize(unsigned int paramIndex, float value) const;

	SingleComponentConfig config;
	std::shared_ptr<::Plugin> plugin;

	FloatBuffer interleavedIn;
	std::vector<FloatBuffer> interleavedOuts;
	Steinberg::int32 numInternalOutputBusses = 1;
};

} // namespace mzglvst
