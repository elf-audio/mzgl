#pragma once

#include "pluginterfaces/gui/iplugview.h"
#include "pluginterfaces/base/funknown.h"
#include "base/source/fobject.h"

#include <memory>

class Plugin;
class PluginEditor;
class Graphics;
class EventDispatcher;

namespace mzglvst {

class MzglVST3PluginProvider {
public:
	virtual ~MzglVST3PluginProvider()					   = default;
	virtual std::shared_ptr<Plugin> getPlugin() const = 0;
};

struct ViewConfig {
	int defaultWidth  = 1024;
	int defaultHeight = 300;
	int minWidth	  = 512;
	int minHeight	  = 150;
	int maxWidth	  = 4096;
	int maxHeight	  = 1200;
};

/**
 * VST3 IPlugView implementation that hosts an mzgl EventsView (the same
 * NSView subclass mzgl's AUv3 wrapper uses) inside the VST3 parent
 * window provided by the host.
 *
 * Lifecycle:
 *   created in the component's createView()
 *   attached() with parent NSView -> create EventsView, add as subview
 *   removed() -> shutdown EventsView, drop the editor
 *   destroyed by VST3 host via FUnknown release
 *
 * The view does NOT own the plugin - the controller does. We borrow it
 * so editor knob values stay in sync with the host's parameter cache.
 */
class MzglVST3View
	: public Steinberg::FObject
	, public Steinberg::IPlugView {
public:
	MzglVST3View(MzglVST3PluginProvider *provider, const ViewConfig &config);
	~MzglVST3View() override;

	OBJ_METHODS(MzglVST3View, FObject)
	REFCOUNT_METHODS(FObject)

	Steinberg::tresult PLUGIN_API queryInterface(const Steinberg::TUID iid, void **obj) SMTG_OVERRIDE;

	// IPlugView
	Steinberg::tresult PLUGIN_API isPlatformTypeSupported(Steinberg::FIDString type) SMTG_OVERRIDE;
	Steinberg::tresult PLUGIN_API attached(void *parent, Steinberg::FIDString type) SMTG_OVERRIDE;
	Steinberg::tresult PLUGIN_API removed() SMTG_OVERRIDE;
	Steinberg::tresult PLUGIN_API onWheel(float distance) SMTG_OVERRIDE { return Steinberg::kResultFalse; }
	Steinberg::tresult PLUGIN_API onKeyDown(Steinberg::char16 key,
											Steinberg::int16 keyCode,
											Steinberg::int16 modifiers) SMTG_OVERRIDE {
		return Steinberg::kResultFalse;
	}
	Steinberg::tresult PLUGIN_API onKeyUp(Steinberg::char16 key,
										  Steinberg::int16 keyCode,
										  Steinberg::int16 modifiers) SMTG_OVERRIDE {
		return Steinberg::kResultFalse;
	}
	Steinberg::tresult PLUGIN_API getSize(Steinberg::ViewRect *size) SMTG_OVERRIDE;
	Steinberg::tresult PLUGIN_API onSize(Steinberg::ViewRect *newSize) SMTG_OVERRIDE;
	Steinberg::tresult PLUGIN_API onFocus(Steinberg::TBool state) SMTG_OVERRIDE { return Steinberg::kResultTrue; }
	Steinberg::tresult PLUGIN_API setFrame(Steinberg::IPlugFrame *frame) SMTG_OVERRIDE;
	Steinberg::tresult PLUGIN_API canResize() SMTG_OVERRIDE { return Steinberg::kResultTrue; }
	Steinberg::tresult PLUGIN_API checkSizeConstraint(Steinberg::ViewRect *rect) SMTG_OVERRIDE;

private:
	struct Impl;
	std::unique_ptr<Impl> impl;

	MzglVST3PluginProvider *provider {nullptr};
	ViewConfig config;

	Steinberg::IPlugFrame *plugFrame {nullptr};
	Steinberg::ViewRect viewRect {0, 0, 1024, 300};
};

} // namespace mzglvst
