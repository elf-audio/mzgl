#import <Cocoa/Cocoa.h>
#include <dlfcn.h>

#include "MzglVST3View.h"

#include "Plugin.h"
#include "PluginEditor.h"
#include "Graphics.h"
#include "EventDispatcher.h"
#include "EventsView.h"
#include "util.h"
#include "filesystem.h"

#if defined(MZGL_SOKOL) || defined(MZGL_METAL)
#	import <MetalKit/MetalKit.h>
#else
#	import <CoreVideo/CoreVideo.h>
#endif

@interface MzglVST3EventsView : EventsView
@property (nonatomic, copy) void (^onFirstDraw)(void);
@end

@implementation MzglVST3EventsView
#if defined(MZGL_SOKOL) || defined(MZGL_METAL)
- (void)drawInMTKView:(MTKView *)view {
	[super drawInMTKView:view];
#else
- (void)renderForTime:(CVTimeStamp)time {
	[super renderForTime:time];
#endif
	if (self.onFirstDraw && [self getEventDispatcher]->hasSetup()) {
		auto fn			 = self.onFirstDraw;
		self.onFirstDraw = nil;
		fn();
	}
}
@end

namespace {
// Tell mzgl where to find data/ inside our .vst3 bundle. libmzgl was
// built without MZGL_PLUGIN_VST, so its baked-in dataPath() falls back
// to "../data/<file>" - which resolves to the host's CWD. Override.
void overrideDataPathToBundleResources() {
	Dl_info info;
	if (!dladdr((void *) &overrideDataPathToBundleResources, &info)) return;
	if (!info.dli_fname) return;
	// Binary is at <Plugin>.vst3/Contents/MacOS/<Plugin> - go up 2 levels
	// then into Resources/data.
	fs::path bundle = fs::path(info.dli_fname).parent_path().parent_path();
	fs::path data	= bundle / "Resources" / "data";
	setDataPath(data.string());
}

CGFloat backingScaleForView(NSView *view) {
	if (view != nil && view.window != nil) {
		return view.window.backingScaleFactor;
	}
	NSScreen *screen = NSScreen.mainScreen;
	if (screen != nil) {
		return screen.backingScaleFactor;
	}
	return 1.0;
}
} // namespace

using namespace Steinberg;

namespace mzglvst {

struct MzglVST3View::Impl {
	std::shared_ptr<Plugin> plugin;
	std::shared_ptr<PluginEditor> editor;
	std::shared_ptr<Graphics> graphics;
	std::shared_ptr<EventDispatcher> dispatcher;
	MzglVST3EventsView *eventsView {nil};
	NSView *parent {nil};

	void teardown() {
		if (editor) {
			editor->pluginViewDisappeared();
		}
		if (eventsView) {
			[eventsView shutdown];
			[eventsView removeFromSuperview];
			eventsView = nil;
		}
		dispatcher.reset();
		editor.reset();
		graphics.reset();
		plugin.reset();
		parent = nil;
	}
};

MzglVST3View::MzglVST3View(MzglVST3PluginProvider *provider, const ViewConfig &_config)
	: impl(std::make_unique<Impl>())
	, provider(provider)
	, config(_config) {
	viewRect = {0, 0, config.defaultWidth, config.defaultHeight};
}

MzglVST3View::~MzglVST3View() {
	if (impl) impl->teardown();
}

tresult PLUGIN_API MzglVST3View::queryInterface(const TUID iid, void **obj) {
	QUERY_INTERFACE(iid, obj, FUnknown::iid, IPlugView)
	QUERY_INTERFACE(iid, obj, IPlugView::iid, IPlugView)
	*obj = nullptr;
	return kNoInterface;
}

tresult PLUGIN_API MzglVST3View::isPlatformTypeSupported(FIDString type) {
	if (type && strcmp(type, kPlatformTypeNSView) == 0) {
		return kResultTrue;
	}
	return kResultFalse;
}

tresult PLUGIN_API MzglVST3View::attached(void *parent, FIDString type) {
	if (!parent || !type) return kInvalidArgument;
	if (strcmp(type, kPlatformTypeNSView) != 0) return kResultFalse;

	overrideDataPathToBundleResources();

	@autoreleasepool {
		impl->parent = (__bridge NSView *) parent;

		// Borrow the controller's plugin so user knob movements and host
		// parameter automation can flow through the same synth instance.
		impl->plugin   = provider ? provider->getPlugin() : instantiatePlugin();
		impl->graphics = std::make_shared<Graphics>();
		impl->editor   = instantiatePluginEditor(*impl->graphics, impl->plugin);
		impl->dispatcher = std::make_shared<EventDispatcher>(impl->editor);

		const NSRect frame = NSMakeRect(0, 0, viewRect.getWidth(), viewRect.getHeight());

		// Pixel-scale aware - the host gives us view-coords (points); mzgl
		// wants pixel coords for g.width/g.height, so scale by the display's
		// real backing factor (1x, 2x, ...) rather than assuming Retina.
		const CGFloat scale		   = backingScaleForView(impl->parent);
		impl->graphics->width	   = viewRect.getWidth() * scale;
		impl->graphics->height	   = viewRect.getHeight() * scale;
		impl->graphics->pixelScale = scale;

		impl->eventsView = [[MzglVST3EventsView alloc] initWithFrame:frame
													   eventDispatcher:impl->dispatcher];
		// In a VST3 host (Live, etc.), the host owns global keyboard
		// shortcuts. Don't steal them.
		impl->eventsView.handlesKeyboard = NO;
		// We're embedded in the host's window: never treat clicks as a
		// window-drag and track the parent's size like the AUv3 path does.
		impl->eventsView.embeddedInHost	  = YES;
		impl->eventsView.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
		[impl->parent addSubview:impl->eventsView];

		impl->dispatcher->resized();
		if (impl->dispatcher->hasSetup()) {
			impl->editor->pluginViewAppeared();
		} else {
			impl->eventsView.onFirstDraw = [editor = impl->editor]() { editor->pluginViewAppeared(); };
		}
	}
	return kResultTrue;
}

tresult PLUGIN_API MzglVST3View::removed() {
	@autoreleasepool {
		impl->teardown();
	}
	return kResultTrue;
}

tresult PLUGIN_API MzglVST3View::getSize(ViewRect *size) {
	if (!size) return kInvalidArgument;
	*size = viewRect;
	return kResultTrue;
}

tresult PLUGIN_API MzglVST3View::onSize(ViewRect *newSize) {
	if (!newSize) return kInvalidArgument;
	viewRect = *newSize;
	@autoreleasepool {
		if (impl->eventsView) {
			NSRect frame = NSMakeRect(0, 0, viewRect.getWidth(), viewRect.getHeight());
			impl->eventsView.frame = frame;
			if (impl->graphics) {
				const CGFloat scale		   = backingScaleForView(impl->eventsView);
				impl->graphics->width	   = viewRect.getWidth() * scale;
				impl->graphics->height	   = viewRect.getHeight() * scale;
				impl->graphics->pixelScale = scale;
			}
			if (impl->dispatcher) impl->dispatcher->resized();
		}
	}
	return kResultTrue;
}

tresult PLUGIN_API MzglVST3View::setFrame(IPlugFrame *frame) {
	plugFrame = frame;
	return kResultTrue;
}

tresult PLUGIN_API MzglVST3View::checkSizeConstraint(ViewRect *rect) {
	if (!rect) return kInvalidArgument;
	int width  = rect->right - rect->left;
	int height = rect->bottom - rect->top;

	if (width < config.minWidth) width = config.minWidth;
	if (width > config.maxWidth) width = config.maxWidth;
	if (height < config.minHeight) height = config.minHeight;
	if (height > config.maxHeight) height = config.maxHeight;

	rect->right	 = rect->left + width;
	rect->bottom = rect->top + height;
	return kResultTrue;
}

} // namespace mzglvst
