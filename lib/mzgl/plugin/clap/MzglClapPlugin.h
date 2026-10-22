#pragma once

// CLAP wrapper for an mzgl ::Plugin + PluginEditor - the CLAP counterpart of
// MzglVST3SingleComponent. The product supplies one ClapSpec (and the usual
// instantiatePlugin()/instantiatePluginEditor()); MzglClapPlugin.cpp turns it
// into a clap_entry with a single plugin.
//
// Scope: Linux today (the editor embeds via plugin/linux/X11PluginView and the
// host's timer/posix-fd extensions). The DSP/params/state code is platform-
// agnostic; adding mac/win means a gui adapter for their native views.

#include "../PluginViewConfig.h"

#include <functional>
#include <memory>
#include <vector>

class Plugin;

namespace mzglclap {

	struct ClapSpec {
		// clap_plugin_descriptor fields (pointers must stay valid for the process
		// lifetime - string literals).
		const char *id			= nullptr; // e.g. "com.elf-audio.koala"
		const char *name		= nullptr;
		const char *vendor		= nullptr;
		const char *url			= nullptr;
		const char *version		= nullptr;
		const char *description = nullptr;
		// CLAP_PLUGIN_FEATURE_* strings, e.g. {"instrument", "sampler"}.
		std::vector<const char *> features;

		bool audioInput		= true; // one stereo in port
	bool midiInput		= true; // one note port, raw MIDI dialect (CC included)
	bool needsTransport = true;

	// Called on clap activate/deactivate with the plugin instance - the CLAP
	// counterpart of MzglVST3SingleComponent::onActivate (KoalaFX wires
	// isRunning here so preset changes apply directly while inactive).
	std::function<void(const std::shared_ptr<Plugin> &, bool)> onActivate;

	// Editor sizing: the static default, or a getter evaluated when the gui
	// is created (for products that size to the screen, like Koala).
	mzglvst::ViewConfig view;
	std::function<mzglvst::ViewConfig()> makeViewConfig;
	};

	// Implemented by the product (next to its factory/entry-points file).
	const ClapSpec &mzglClapSpec();

} // namespace mzglclap
