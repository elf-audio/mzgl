// AUv2 (.component): subclass mzgl's generic AudioUnitSDK-based wrapper and
// declare the component entry point. AUSDK_COMPONENT_ENTRY(..., GainAUv2)
// defines `GainAUv2Factory`, which is the FACTORY_FUNCTION named in
// CMakeLists.txt (it ends up in the bundle's Info.plist).
#include "MzglAUv2.h"
#include "GainPlugin.h"
#include "GainEditor.h"

#include <AudioUnitSDK/AUPlugInDispatch.h>

class GainAUv2 : public mzglau::MzglAUv2Effect {
public:
	explicit GainAUv2(AudioComponentInstance ci)
		: MzglAUv2Effect(ci, std::make_shared<GainPlugin>(), config()) {}

private:
	static mzglau::AUv2Config config() {
		mzglau::AUv2Config cfg;
		cfg.midiInput  = false;
		cfg.viewWidth  = GainEditor::kDefaultWidth;
		cfg.viewHeight = GainEditor::kDefaultHeight;
		cfg.minWidth   = 160;
		cfg.minHeight  = 240;
		cfg.maxWidth   = 1200;
		cfg.maxHeight  = 1800;
		return cfg;
	}
};

// AUBaseFactory for an effect (aufx). Use ausdk::AUMIDIEffectFactory for a
// MIDI-controlled effect (aumf) so hosts can send it MIDI.
AUSDK_COMPONENT_ENTRY(ausdk::AUBaseFactory, GainAUv2)
