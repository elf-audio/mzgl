// mzgl entry points. The AUv3 (mzgl's AudioUnitViewController) calls
// instantiatePlugin() when the host creates the audio unit and
// instantiatePluginEditor() when the view first appears; the VST3/AUv2 wrappers
// call instantiatePluginEditor() too. isPlugin() is consulted by mzgl's iOS
// app delegate. instantiateApp() is the standalone-app entry point that mzgl
// references unconditionally; a plugin never runs as an app, so return null.
#include "Plugin.h"
#include "PluginEditor.h"
#include "App.h"

#include "GainPlugin.h"
#include "GainEditor.h"

bool isPlugin() {
	return true;
}

std::shared_ptr<Plugin> instantiatePlugin() {
	return std::make_shared<GainPlugin>();
}

std::shared_ptr<PluginEditor> instantiatePluginEditor(Graphics &g, std::shared_ptr<Plugin> plugin) {
	return std::make_shared<GainEditor>(g, plugin);
}

std::shared_ptr<App> instantiateApp(Graphics &g) {
	return nullptr;
}
