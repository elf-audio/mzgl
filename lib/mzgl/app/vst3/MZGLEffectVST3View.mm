#include "MZGLEffectVST3View.h"
#include "MZGLEffectVST3.h"
#include "Plugin.h"
#include "PluginEditor.h"
#include "EventDispatcher.h"

#import <Cocoa/Cocoa.h>
#import "EventsView.h"

using namespace Steinberg;

MZGLEffectVST3View::MZGLEffectVST3View(MZGLEffectVST3 *processor)
	: processor(processor) {
	viewWidth  = kDefaultWidth;
	viewHeight = kDefaultHeight;

	rect.left	= 0;
	rect.top	= 0;
	rect.right	= viewWidth;
	rect.bottom = viewHeight;
}

MZGLEffectVST3View::~MZGLEffectVST3View() {
	removed();
}

tresult PLUGIN_API MZGLEffectVST3View::isPlatformTypeSupported(FIDString type) {
	if (FIDStringsEqual(type, kPlatformTypeNSView)) {
		return kResultTrue;
	}
	return kResultFalse;
}

tresult PLUGIN_API MZGLEffectVST3View::attached(void *parent, FIDString type) {
	if (!FIDStringsEqual(type, kPlatformTypeNSView)) {
		return kResultFalse;
	}

	NSView *parentView = (NSView *) parent;
	if (!parentView) {
		return kResultFalse;
	}

	graphics = std::make_shared<Graphics>();

	float pixelScale	 = [[NSScreen mainScreen] backingScaleFactor];
	graphics->width		 = viewWidth * pixelScale;
	graphics->height	 = viewHeight * pixelScale;
	graphics->pixelScale = pixelScale;

	auto plugin = processor->getPlugin();
	if (!plugin) {
		return kResultFalse;
	}

	pluginEditor = instantiatePluginEditor(*graphics, plugin);
	if (!pluginEditor) {
		return kResultFalse;
	}

	eventDispatcher = std::make_shared<EventDispatcher>(pluginEditor);

	NSRect frame = NSMakeRect(0, 0, viewWidth, viewHeight);

	EventsView *eventsView = [[EventsView alloc] initWithFrame:frame
											   eventDispatcher:eventDispatcher
												withRenderMode:RenderMode::UseRenderTimer];

	[parentView addSubview:eventsView];

	NSRect parentBounds = parentView.bounds;
	if (parentBounds.size.width > 0 && parentBounds.size.height > 0) {
		eventsView.frame = parentBounds;
	}

	[eventsView retain];
	nativeView = (void *) eventsView;

	pluginEditor->pluginViewAppeared();

	return kResultTrue;
}

tresult PLUGIN_API MZGLEffectVST3View::removed() {
	if (nativeView) {
		if (pluginEditor) {
			pluginEditor->pluginViewDisappeared();
		}

		EventsView *eventsView = (EventsView *) nativeView;
		[eventsView shutdown];
		[eventsView removeFromSuperview];
		[eventsView release];
		nativeView = nullptr;
	}

	eventDispatcher.reset();
	pluginEditor.reset();
	graphics.reset();

	return kResultTrue;
}

#include "mzAssert.h"

tresult PLUGIN_API MZGLEffectVST3View::onSize(ViewRect *newSize) {
	if (!newSize) {
		return kResultFalse;
	}

	viewWidth  = newSize->getWidth();
	viewHeight = newSize->getHeight();

	if (nativeView) {
		EventsView *eventsView = (EventsView *) nativeView;
		NSView *parentView	   = eventsView.superview;
		if (parentView) {
			NSRect parentBounds = parentView.bounds;
			if (parentBounds.size.width > 0 && parentBounds.size.height > 0) {
				eventsView.frame = parentBounds;
			}
		}

		if (graphics) {
			float pixelScale	 = [[NSScreen mainScreen] backingScaleFactor];
			graphics->width		 = viewWidth * pixelScale;
			graphics->height	 = viewHeight * pixelScale;
			graphics->pixelScale = pixelScale;
		}

		if (eventDispatcher) {
			eventDispatcher->resized();
		}
	}

	return kResultTrue;
}

tresult PLUGIN_API MZGLEffectVST3View::getSize(ViewRect *size) {
	if (!size) {
		return kResultFalse;
	}

	if (pluginEditor) {
		auto dims = pluginEditor->getPreferredDimensions();

		float pixelScale = [[NSScreen mainScreen] backingScaleFactor];
		viewWidth		 = dims.first / pixelScale;
		viewHeight		 = dims.second / pixelScale;
	}

	size->left	 = 0;
	size->top	 = 0;
	size->right	 = viewWidth;
	size->bottom = viewHeight;

	return kResultTrue;
}

tresult PLUGIN_API MZGLEffectVST3View::canResize() {
	return kResultTrue;
}

tresult PLUGIN_API MZGLEffectVST3View::checkSizeConstraint(ViewRect *rect) {
	if (!rect) {
		return kResultFalse;
	}

	constexpr int minWidth	= 320;
	constexpr int minHeight = 480;

	int width  = rect->getWidth();
	int height = rect->getHeight();

	if (width < minWidth) {
		rect->right = rect->left + minWidth;
	}
	if (height < minHeight) {
		rect->bottom = rect->top + minHeight;
	}

	return kResultTrue;
}

tresult PLUGIN_API MZGLEffectVST3View::onWheel(float /*distance*/) {
	return kResultFalse;
}

tresult PLUGIN_API MZGLEffectVST3View::onKeyDown(char16 /*key*/, int16 /*keyCode*/, int16 /*modifiers*/) {
	return kResultFalse;
}

tresult PLUGIN_API MZGLEffectVST3View::onKeyUp(char16 /*key*/, int16 /*keyCode*/, int16 /*modifiers*/) {
	return kResultFalse;
}

tresult PLUGIN_API MZGLEffectVST3View::onFocus(TBool /*state*/) {
	return kResultOk;
}

tresult PLUGIN_API MZGLEffectVST3View::setFrame(IPlugFrame *frame) {
	plugFrame = frame;
	return kResultOk;
}
