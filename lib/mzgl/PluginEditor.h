//
//  Plugin.h
//  auv3test
//
//  Created by Marek Bereza on 18/01/2018.
//  Copyright © 2018 Marek Bereza. All rights reserved.
//

#pragma once
#include "App.h"
#include "Plugin.h"

class PluginEditor : public App {
public:
	::Plugin &effect;

	PluginEditor(Graphics &g, ::Plugin &effect)
		: App(g)
		, effect(effect) {}
	virtual ~PluginEditor() {}

	// right now just for auv3
	// so koala knows when to cut off the otherMain
	// - the plugin can be started without the view,
	// or it can temporarily stop the view from rendering
	// in which case we don't get a main loop
	virtual void pluginViewAppeared() {}
	virtual void pluginViewDisappeared() {}

	virtual std::pair<int, int> getPreferredDimensions() const = 0;

	std::function<void(unsigned int, float)> uiParameterChanged;
};

// entry point for plugin editor
std::shared_ptr<PluginEditor> instantiatePluginEditor(Graphics &g, std::shared_ptr<Plugin> plugin);
// Supplied by the app: true if it starts through instantiatePlugin() +
// instantiatePluginEditor() rather than instantiateApp(). That is how the app is
// built, not where it runs - a standalone app can return true, so this does not
// mean "embedded in a host".
bool usesMZGLPluginEntryPoints();
