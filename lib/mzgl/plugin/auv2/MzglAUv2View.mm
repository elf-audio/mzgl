// AUv2 Cocoa editor: the AUCocoaUIBase view factory the host asks for via
// kAudioUnitProperty_CocoaUI, and the NSView it returns, which hosts an mzgl
// EventsView + PluginEditor exactly like MzglVST3View.mm does for VST3.
//
// The view runs in the host process and talks to the C++ audio unit (an
// MzglAUv2Effect or MzglAUv2Instrument, seen through MzglAUv2Host) via
// kMzglAUv2Property_Instance, sharing its ::Plugin with the DSP.
//
// Resizing: the view carries flexible autoresizing masks and re-lays out on
// every frame change, so hosts that let the user drag AUv2 windows (Logic,
// Live, Reaper, ...) get a live relayout for any size/aspect.
#import <Cocoa/Cocoa.h>
#import <AudioUnit/AUCocoaUIView.h>

#include "MzglAUv2.h"
#include "MzglAUv2View.h"

#include "Plugin.h"
#include "PluginEditor.h"
#include "Graphics.h"
#include "EventDispatcher.h"
#include "EventsView.h"

#if defined(MZGL_SOKOL) || defined(MZGL_METAL)
#	import <MetalKit/MetalKit.h>
#else
#	import <CoreVideo/CoreVideo.h>
#endif

namespace {
CGFloat backingScaleForView(NSView *view) {
	if (view != nil && view.window != nil) return view.window.backingScaleFactor;
	NSScreen *screen = NSScreen.mainScreen;
	return screen != nil ? screen.backingScaleFactor : 1.0;
}
} // namespace

// ---------------------------------------------------------------------------
// EventsView subclass: fires a block on the first frame after mzgl's setup ran.
// ---------------------------------------------------------------------------
@interface MZGL_AUV2_EVENTS_VIEW_CLASS : EventsView
@property(nonatomic, copy) void (^onFirstDraw)(void);
@end

@implementation MZGL_AUV2_EVENTS_VIEW_CLASS
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

// ---------------------------------------------------------------------------
// Host view
// ---------------------------------------------------------------------------
@interface MZGL_AUV2_HOST_VIEW_CLASS : NSView
- (instancetype)initWithAudioUnit:(mzglau::MzglAUv2Host *)host;
@end

@implementation MZGL_AUV2_HOST_VIEW_CLASS {
	std::shared_ptr<Plugin> plugin;
	std::shared_ptr<PluginEditor> editor;
	std::shared_ptr<Graphics> graphics;
	std::shared_ptr<EventDispatcher> dispatcher;
	MZGL_AUV2_EVENTS_VIEW_CLASS *eventsView;
	mzglau::AUv2Config config;
}

- (instancetype)initWithAudioUnit:(mzglau::MzglAUv2Host *)host {
	config		= host->getConfig();
	NSRect frame = NSMakeRect(0, 0, config.viewWidth, config.viewHeight);
	self		 = [super initWithFrame:frame];
	if (self == nil) return nil;

	self.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;

	plugin	   = host->getPlugin();
	graphics   = std::make_shared<Graphics>();
	editor	   = instantiatePluginEditor(*graphics, plugin);
	dispatcher = std::make_shared<EventDispatcher>(editor);

	const CGFloat scale	 = backingScaleForView(self);
	graphics->width		 = config.viewWidth * scale;
	graphics->height	 = config.viewHeight * scale;
	graphics->pixelScale = scale;

	eventsView					= [[MZGL_AUV2_EVENTS_VIEW_CLASS alloc] initWithFrame:self.bounds eventDispatcher:dispatcher];
	eventsView.handlesKeyboard	= NO; // the host owns global shortcuts
	eventsView.embeddedInHost	= YES;
	eventsView.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
	[self addSubview:eventsView];

	dispatcher->resized();
	if (dispatcher->hasSetup()) {
		editor->pluginViewAppeared();
	} else {
		auto ed				   = editor;
		eventsView.onFirstDraw = ^{ ed->pluginViewAppeared(); };
	}
	return self;
}

- (void)relayout {
	if (eventsView == nil || graphics == nullptr) return;
	eventsView.frame	 = self.bounds;
	const CGFloat scale	 = backingScaleForView(self);
	graphics->width		 = self.bounds.size.width * scale;
	graphics->height	 = self.bounds.size.height * scale;
	graphics->pixelScale = scale;
	if (dispatcher) dispatcher->resized();
}

- (void)setFrameSize:(NSSize)newSize {
	[super setFrameSize:newSize];
	[self relayout];
}

- (void)viewDidChangeBackingProperties {
	[super viewDidChangeBackingProperties];
	[self relayout];
}

- (void)teardown {
	if (editor) editor->pluginViewDisappeared();
	if (eventsView != nil) {
		[eventsView shutdown];
		[eventsView removeFromSuperview];
		eventsView = nil;
	}
	dispatcher.reset();
	editor.reset();
	graphics.reset();
	plugin.reset();
}

- (void)removeFromSuperview {
	[self teardown];
	[super removeFromSuperview];
}

- (void)dealloc {
	[self teardown];
}

@end

// ---------------------------------------------------------------------------
// View factory (AUCocoaUIBase) - the class named in kAudioUnitProperty_CocoaUI.
// ---------------------------------------------------------------------------
@interface MZGL_AUV2_VIEW_FACTORY_CLASS : NSObject <AUCocoaUIBase>
@end

@implementation MZGL_AUV2_VIEW_FACTORY_CLASS

- (unsigned)interfaceVersion {
	return 0;
}

- (NSString *)description {
	return @"mzgl AUv2 view";
}

- (NSView *)uiViewForAudioUnit:(AudioUnit)inAU withSize:(NSSize)inPreferredSize {
	mzglau::MzglAUv2Host *host = nullptr;
	UInt32 size				   = sizeof(host);
	if (AudioUnitGetProperty(inAU, mzglau::kMzglAUv2Property_Instance, kAudioUnitScope_Global, 0, &host, &size)
			!= noErr
		|| host == nullptr) {
		return nil;
	}
	return [[MZGL_AUV2_HOST_VIEW_CLASS alloc] initWithAudioUnit:host];
}

@end
