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

class PluginEditor: public App {
public:
	::Plugin &effect;
	
	PluginEditor(Graphics &g, ::Plugin &effect) : App(g), effect(effect) {}
	virtual ~PluginEditor() {}


	std::function<void(unsigned int, float)> uiParameterChanged;
};



// entry point for plugin editor
PluginEditor *instantiatePluginEditor(Graphics &g, Plugin *plugin);
bool isPlugin();
