#pragma once

#include "App.h"

// Which hardware-keyboard events a plugin editor embedded in a host keeps for
// itself. The host owns its shortcuts, so every platform view asks here before
// swallowing a key and passes anything refused on to the host:
//  - nothing with the command key held (cmd on Apple platforms, ctrl on
//    Windows/Linux): undo/redo, select all, save... stay the host's
//  - not the space bar: that is the host's transport
//  - nothing at all unless the editor opts in (App::pluginAcceptsKeyEvents)
// While a text field is being edited the editor gets everything but the command
// combos, whatever it opted into - typing a name needs the space bar.
namespace mzgl {

	inline bool pluginShouldCaptureKey(const App &app, int key, bool commandDown) {
		if (commandDown) return false;
		if (app.g.textInputReceiver != nullptr) return true;
		if (!app.pluginAcceptsKeyEvents()) return false;
		return key != ' ';
	}

	// Whether the editor's view should hold keyboard focus at all right now.
	inline bool pluginWantsKeyboardFocus(const App &app) {
		return app.g.textInputReceiver != nullptr || app.pluginAcceptsKeyEvents();
	}

} // namespace mzgl
