#pragma once

#include "pluginterfaces/gui/iplugview.h"
#include "pluginterfaces/base/funknown.h"
#include "base/source/fobject.h"
#ifdef _WIN32
#	include "pluginterfaces/gui/iplugviewcontentscalesupport.h"
#endif

#include <cstdint>
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
 * VST3 IPlugView implementation that hosts the mzgl editor inside the parent
 * window provided by the host.
 *
 *   macOS   (MzglVST3View.mm)      an mzgl EventsView (the same NSView subclass
 *                                  mzgl's AUv3 wrapper uses) added as a subview
 *                                  of the host's NSView
 *   Windows (MzglVST3View_win.cpp) a child HWND of the host's HWND, rendered
 *                                  with sokol/D3D11 off a WM_TIMER, Win32
 *                                  messages translated to EventDispatcher calls
 *
 * Lifecycle:
 *   created in the component's createView()
 *   attached() with the parent view/window -> create the native view, editor
 *   removed() -> shut the native view down, drop the editor
 *   destroyed by VST3 host via FUnknown release
 *
 * Sizes: ViewRect is in points on macOS and in physical pixels on Windows
 * (the VST3 convention - the host passes the DPI scale separately through
 * IPlugViewContentScaleSupport). Either way Graphics gets pixel dimensions
 * plus the scale in pixelScale.
 *
 * The view does NOT own the plugin - the controller does. We borrow it
 * so editor knob values stay in sync with the host's parameter cache.
 */
class MzglVST3View
	: public Steinberg::FObject
	, public Steinberg::IPlugView
#ifdef _WIN32
	, public Steinberg::IPlugViewContentScaleSupport
#endif
{
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

#ifdef _WIN32
	// IPlugViewContentScaleSupport - the host's DPI scale for the window we're
	// in (1.0 = 96 dpi). ViewRect stays in physical pixels; this only changes
	// how big the UI draws (Graphics::pixelScale) and our preferred size.
	Steinberg::tresult PLUGIN_API setContentScaleFactor(ScaleFactor factor) SMTG_OVERRIDE;
#endif

private:
	struct Impl;
	std::unique_ptr<Impl> impl;

	MzglVST3PluginProvider *provider {nullptr};
	ViewConfig config;

	Steinberg::IPlugFrame *plugFrame {nullptr};
	Steinberg::ViewRect viewRect {0, 0, 1024, 300};

#ifdef _WIN32
	// DPI scale (physical px per logical px). ViewConfig is in logical px.
	float contentScale {1.f};
	bool hostSetContentScale {false};
	Steinberg::ViewRect scaledRect(int logicalW, int logicalH) const;
	void renderFrame();
	// Win32 message handler for the child window (wndProc in the .cpp forwards
	// here). Plain integer types so this header stays free of <windows.h>.
	std::intptr_t handleMessage(unsigned msg, std::uintptr_t wParam, std::intptr_t lParam, bool &handled);
	friend struct MzglVST3ViewWin32;
#endif
};

} // namespace mzglvst
