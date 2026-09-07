// VST3: subclass mzgl's generic single-component wrapper (one object is both
// the audio processor and the edit controller, sharing ONE GainPlugin with the
// editor), then declare the class factory the host queries.
#include "MzglVST3SingleComponent.h"
#include "GainPlugin.h"
#include "GainEditor.h"
#include "version.h"

#include "public.sdk/source/main/pluginfactory.h"
#include "pluginterfaces/vst/ivstaudioprocessor.h"

namespace {

// Generate a fresh one per plugin (e.g. `uuidgen`) and NEVER change it: hosts
// identify the plugin in saved projects by this ID.
const Steinberg::FUID kGainVST3UID(0x3F8A2C51, 0x9B7E4D2A, 0xA1C6E093, 0x5D7B8F24);

class GainVST3 : public mzglvst::MzglVST3SingleComponent {
public:
	static Steinberg::FUnknown *createInstance(void *) {
		return static_cast<Steinberg::Vst::IAudioProcessor *>(new GainVST3());
	}

protected:
	std::shared_ptr<Plugin> createPlugin() override { return std::make_shared<GainPlugin>(); }

	mzglvst::SingleComponentConfig getConfig() const override {
		mzglvst::SingleComponentConfig cfg;
		cfg.audioInput		   = true; // effect; false for an instrument
		cfg.midiInput		   = false; // true to receive notes in midiReceivedAtTime()
		cfg.midiCCProxies	   = false; // true to expose per-channel CC proxies (IMidiMapping)
		cfg.needsTransport	   = false; // true to have plugin->bpm / beatPosition filled in
		cfg.view.defaultWidth  = GainEditor::kDefaultWidth;
		cfg.view.defaultHeight = GainEditor::kDefaultHeight;
		cfg.view.minWidth	   = 160;
		cfg.view.minHeight	   = 240;
		cfg.view.maxWidth	   = 1200;
		cfg.view.maxHeight	   = 1800;
		return cfg;
	}
};

} // namespace

BEGIN_FACTORY_DEF(stringCompanyName, stringCompanyWeb, stringCompanyEmail)

DEF_CLASS2(INLINE_UID_FROM_FUID(kGainVST3UID),
		   PClassInfo::kManyInstances,
		   kVstAudioEffectClass,
		   stringPluginName,
		   Steinberg::Vst::kDistributable,
		   Steinberg::Vst::PlugType::kFxDynamics,
		   GAIN_VERSION_STR,
		   kVstVersionString,
		   GainVST3::createInstance)

END_FACTORY
