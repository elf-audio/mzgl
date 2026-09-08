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

#include <algorithm>

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
// Resize grip - a small bottom-right corner control (like the diagonal lines in
// Chow/JUCE plugins). AUv2 windows aren't drag-resizable in every host (Logic
// lets you drag the edge, Ableton Live doesn't), so we draw our own grabber and
// resize the host view + its window on drag, clamped to the config's min/max.
@interface MZGL_AUV2_HOST_VIEW_CLASS : NSView
- (void)userResizeToWidth:(CGFloat)w height:(CGFloat)h;
- (instancetype)initWithAudioUnit:(mzglau::MzglAUv2Host *)host;
@end

@interface MZGL_AUV2_RESIZE_GRIP_CLASS : NSView
@property(nonatomic, weak) MZGL_AUV2_HOST_VIEW_CLASS *hostView;
@end

@implementation MZGL_AUV2_RESIZE_GRIP_CLASS {
	NSPoint _startMouseScreen; // screen coords: unaffected by the window resizing mid-drag
	NSSize _startHostSize;
}

- (BOOL)isFlipped { return NO; }

// A stroked quarter circle hugging the bottom-right corner, like the iOS resize
// affordance. Non-flipped coords (origin bottom-left), so the window's corner is
// this view's bottom-right; centre the arc there and sweep the top-left quarter.
- (void)drawRect:(NSRect)dirty {
	const CGFloat w			= self.bounds.size.width;
	const CGFloat thickness = 4.5;			// 1.5x the previous 3.0
	const CGFloat inset		= thickness;	// gap from the corner == the stroke thickness
	const CGFloat radius	= 12.0;			// 1.5x the previous 8.0
	// Rounded-rect bottom-right corner: arc centre is up-left of the corner, so
	// the curve is convex toward the bottom-right. Non-flipped coords (origin
	// bottom-left), so bottom = y 0, right = x w. Sweep 270deg (down) -> 360deg
	// (right), which traces the bottom-right quarter.
	const NSPoint centre = NSMakePoint(w - inset - radius, inset + radius);
	NSBezierPath *p = [NSBezierPath bezierPath];
	p.lineWidth		= thickness;
	p.lineCapStyle	= NSLineCapStyleRound;
	[p appendBezierPathWithArcWithCenter:centre radius:radius startAngle:270 endAngle:360];
	[[NSColor colorWithWhite:1.0 alpha:0.5] setStroke];
	[p stroke];
}

- (void)resetCursorRects {
	// AppKit has no public diagonal-resize cursor; the private one is what the
	// system uses for window corners. Fall back to crosshair if it ever goes away.
	NSCursor *cursor		= nil;
	SEL diagonalResizeSel = NSSelectorFromString(@"_windowResizeNorthWestSouthEastCursor");
	if ([NSCursor respondsToSelector:diagonalResizeSel]) {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Warc-performSelector-leaks"
		cursor = [NSCursor performSelector:diagonalResizeSel];
#pragma clang diagnostic pop
	}
	if (cursor == nil) cursor = [NSCursor crosshairCursor];
	[self addCursorRect:self.bounds cursor:cursor];
}

- (void)mouseDown:(NSEvent *)event {
	_startMouseScreen = [NSEvent mouseLocation];
	_startHostSize	  = self.hostView ? self.hostView.frame.size : NSZeroSize;
}

- (void)mouseDragged:(NSEvent *)event {
	if (self.hostView == nil) return;
	const NSPoint m	 = [NSEvent mouseLocation]; // absolute screen coords
	const CGFloat dx = m.x - _startMouseScreen.x;
	const CGFloat dy = m.y - _startMouseScreen.y; // screen y is up; dragging DOWN => dy<0 => taller
	[self.hostView userResizeToWidth:_startHostSize.width + dx height:_startHostSize.height - dy];
}
@end

// ---------------------------------------------------------------------------
// Host view
// ---------------------------------------------------------------------------
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

	// Corner resize grabber, pinned bottom-right, above the GL view.
	const CGFloat gripSize = 30;
	MZGL_AUV2_RESIZE_GRIP_CLASS *grip = [[MZGL_AUV2_RESIZE_GRIP_CLASS alloc]
		initWithFrame:NSMakeRect(self.bounds.size.width - gripSize, 0, gripSize, gripSize)];
	grip.hostView		= self;
	grip.autoresizingMask = NSViewMinXMargin | NSViewMaxYMargin; // stay bottom-right
	[self addSubview:grip];

	dispatcher->resized();
	if (dispatcher->hasSetup()) {
		editor->pluginViewAppeared();
	} else {
		auto ed				   = editor;
		eventsView.onFirstDraw = ^{ ed->pluginViewAppeared(); };
	}
	return self;
}

// Drag-resize from the corner grip: clamp to the config, then resize the host
// window (the AU view fills it via autoresizing). Growing from the window's
// bottom-left origin, so the window extends up/right.
- (void)userResizeToWidth:(CGFloat)w height:(CGFloat)h {
	w = std::max((CGFloat) config.minWidth, std::min((CGFloat) config.maxWidth, w));
	h = std::max((CGFloat) config.minHeight, std::min((CGFloat) config.maxHeight, h));
	NSWindow *win = self.window;
	if (win != nil && win.contentView == self) {
		// Resize the window keeping its TOP-LEFT corner fixed: macOS windows are
		// bottom-left anchored, so setContentSize alone grows upward and the
		// dragged bottom corner runs away from the cursor. Convert the wanted
		// content size to a frame size and drop the origin so the top edge stays.
		const NSRect frame		= win.frame;
		const NSSize frameSize	= [win frameRectForContentRect:NSMakeRect(0, 0, w, h)].size;
		const CGFloat topEdge	= frame.origin.y + frame.size.height;
		NSRect nf				= frame;
		nf.size					= frameSize;
		nf.origin.y				= topEdge - frameSize.height;
		[win setFrame:nf display:YES];
	} else {
		[self setFrameSize:NSMakeSize(w, h)]; // fallback: resize just our view
	}
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
