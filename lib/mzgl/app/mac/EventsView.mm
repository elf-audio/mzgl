//
//  EventsView.m
//  mzgl macOS
//
//  Created by Marek Bereza on 03/02/2021.
//  Copyright © 2021 Marek Bereza. All rights reserved.
//

#import "EventsView.h"
#include "util.h"
#include "mzgl/util/log.h"
#include "mainThread.h"

#include "NSEventDispatcher.h"
#include "EventDispatcher.h"

@implementation EventsView {
	NSInteger acceptedDraggingSequenceNo;
	bool dropped;
	bool lastShiftState;
	bool lastFnState;
	bool lastControlState;
	bool lastOptionState;
	bool lastCommandState;
}
- (id)initWithFrame:(NSRect)frame eventDispatcher:(std::shared_ptr<EventDispatcher>)evtDispatcher {
	self = [super initWithFrame:frame eventDispatcher:evtDispatcher];
	if (self != nil) {
		dropped			 = false;
		lastShiftState	 = false;
		lastFnState		 = false;
		lastControlState = false;
		lastOptionState	 = false;
		lastCommandState = false;
		[self registerForDraggedTypes:[NSArray arrayWithObject:NSFilenamesPboardType]];
	}
	return self;
}

- (BOOL)mouseDownCanMoveWindow {
	return YES;
}

- (BOOL)acceptsFirstResponder {
	return YES;
}

- (BOOL)becomeFirstResponder {
	return YES;
}

// this is for vst so the window becomes focused and accepts
// the first mouse event when unfocussed.
- (BOOL)acceptsFirstMouse:(NSEvent *)theEvent {
	return YES;
}

- (BOOL)isOpaque {
	return YES;
}

int nsEventToKey(NSEvent *evt) {
	std::string s = [[evt characters] UTF8String];
	if (s.size() == 1) {
		return s[0];
	}
	auto keyCode = [evt keyCode];
	switch (keyCode) {
		case 123: return MZ_KEY_LEFT;
		case 124: return MZ_KEY_RIGHT;
		case 125: return MZ_KEY_DOWN;
		case 126: return MZ_KEY_UP;
	}
	printf("keycode: %u\n", keyCode);
	return keyCode;
}

- (void)flagsChanged:(NSEvent *)event {
	if ((event.modifierFlags & NSEventModifierFlagShift) && !lastShiftState) {
		lastShiftState = true;

		eventDispatcher->app->main.runOnMainThread(true, [self]() { eventDispatcher->keyDown(MZ_KEY_SHIFT); });
	} else if (!(event.modifierFlags & NSEventModifierFlagShift) && lastShiftState) {
		lastShiftState = false;
		eventDispatcher->app->main.runOnMainThread(true, [self]() { eventDispatcher->keyUp(MZ_KEY_SHIFT); });
	} else if ((event.modifierFlags & NSEventModifierFlagFunction) && !lastFnState) {
		lastFnState = true;
		eventDispatcher->app->main.runOnMainThread(true, [self]() { eventDispatcher->keyDown(MZ_KEY_FN); });

	} else if (!(event.modifierFlags & NSEventModifierFlagFunction) && lastFnState) {
		lastFnState = false;
		//Fn released - do something
		eventDispatcher->app->main.runOnMainThread(true, [self]() { eventDispatcher->keyUp(MZ_KEY_FN); });
	} else if ((event.modifierFlags & NSEventModifierFlagControl) && !lastControlState) {
		lastControlState = true;
		//Control pressed - do something
		eventDispatcher->app->main.runOnMainThread(true, [self]() { eventDispatcher->keyDown(MZ_KEY_CTRL); });
	} else if (!(event.modifierFlags & NSEventModifierFlagControl) && lastControlState) {
		lastControlState = false;
		//Control released - do something
		eventDispatcher->app->main.runOnMainThread(true, [self]() { eventDispatcher->keyUp(MZ_KEY_CTRL); });
	} else if ((event.modifierFlags & NSEventModifierFlagOption) && !lastOptionState) {
		lastOptionState = true;
		//Option pressed - do something
		eventDispatcher->app->main.runOnMainThread(true, [self]() { eventDispatcher->keyDown(MZ_KEY_ALT); });
	} else if (!(event.modifierFlags & NSEventModifierFlagOption) && lastOptionState) {
		lastOptionState = false;
		//Option released - do something
		eventDispatcher->app->main.runOnMainThread(true, [self]() { eventDispatcher->keyUp(MZ_KEY_ALT); });
	} else if ((event.modifierFlags & NSEventModifierFlagCommand) && !lastCommandState) {
		lastCommandState = true;
		//Command pressed - do something
		eventDispatcher->app->main.runOnMainThread(true, [self]() { eventDispatcher->keyDown(MZ_KEY_CMD); });
	} else if (!(event.modifierFlags & NSEventModifierFlagCommand) && lastCommandState) {
		lastCommandState = false;
		//Command released - do something
		eventDispatcher->app->main.runOnMainThread(true, [self]() { eventDispatcher->keyUp(MZ_KEY_CMD); });
	}

	else
		NSLog(@"Other");
}

- (void)keyDown:(NSEvent *)event {
	auto keyCode = nsEventToKey(event);
	eventDispatcher->app->main.runOnMainThread(true, [self, keyCode]() { eventDispatcher->keyDown(keyCode); });
	NSEventDispatcher::instance().dispatch(event, self);
}
- (void)keyUp:(NSEvent *)event {
	auto keyCode = nsEventToKey(event);
	eventDispatcher->app->main.runOnMainThread(true, [self, keyCode]() { eventDispatcher->keyUp(keyCode); });
	NSEventDispatcher::instance().dispatch(event, self);
}
- (vec2)transformMouse:(NSEvent *)event {
	auto titleBarHeight = event.window.frame.size.height - event.window.contentView.frame.size.height;
	float pixelScale	= 2; //eventDispatcher->app->g.pixelScale;
	float x				= event.locationInWindow.x * pixelScale;
	float y = (event.window.frame.size.height - event.locationInWindow.y - titleBarHeight) * pixelScale;
	return vec2(x, y);
}
- (void)mouseMoved:(NSEvent *)event {
	auto mouse = [self transformMouse:event];
	eventDispatcher->app->main.runOnMainThread(true,
											   [self, mouse]() { eventDispatcher->touchOver(mouse.x, mouse.y); });
	NSEventDispatcher::instance().dispatch(event, self);
}

/// ----------------
- (void)mouseDown:(NSEvent *)event {
	auto mouse = [self transformMouse:event];
	eventDispatcher->app->main.runOnMainThread(
		true, [self, mouse]() { eventDispatcher->touchDown(mouse.x, mouse.y, 0); });
	NSEventDispatcher::instance().dispatch(event, self);
}
- (void)rightMouseDown:(NSEvent *)event {
	auto mouse = [self transformMouse:event];
	eventDispatcher->app->main.runOnMainThread(
		true, [self, mouse]() { eventDispatcher->touchDown(mouse.x, mouse.y, RightMouseButton); });
	NSEventDispatcher::instance().dispatch(event, self);
}

- (void)otherMouseDown:(NSEvent *)event {
	auto mouse = [self transformMouse:event];
	eventDispatcher->app->main.runOnMainThread(
		true, [self, mouse]() { eventDispatcher->touchDown(mouse.x, mouse.y, MiddleMouseButton); });
	NSEventDispatcher::instance().dispatch(event, self);
}
/// ------------------------
- (void)mouseUp:(NSEvent *)event {
	auto mouse = [self transformMouse:event];
	eventDispatcher->app->main.runOnMainThread(true,
											   [self, mouse]() { eventDispatcher->touchUp(mouse.x, mouse.y, 0); });
	NSEventDispatcher::instance().dispatch(event, self);
}

- (void)rightMouseUp:(NSEvent *)event {
	auto mouse = [self transformMouse:event];
	eventDispatcher->app->main.runOnMainThread(
		true, [self, mouse]() { eventDispatcher->touchUp(mouse.x, mouse.y, RightMouseButton); });
	NSEventDispatcher::instance().dispatch(event, self);
}

- (void)otherMouseUp:(NSEvent *)event {
	auto mouse = [self transformMouse:event];
	eventDispatcher->app->main.runOnMainThread(
		true, [self, mouse]() { eventDispatcher->touchUp(mouse.x, mouse.y, MiddleMouseButton); });
	NSEventDispatcher::instance().dispatch(event, self);
}

/// ------------------------

- (void)mouseDragged:(NSEvent *)event {
	auto mouse = [self transformMouse:event];

	eventDispatcher->app->main.runOnMainThread(
		true, [self, mouse]() { eventDispatcher->touchMoved(mouse.x, mouse.y, 0); });
	NSEventDispatcher::instance().dispatch(event, self);
}

- (void)rightMouseDragged:(NSEvent *)event {
	auto mouse = [self transformMouse:event];

	eventDispatcher->app->main.runOnMainThread(
		true, [self, mouse]() { eventDispatcher->touchMoved(mouse.x, mouse.y, RightMouseButton); });
	NSEventDispatcher::instance().dispatch(event, self);
}
- (void)otherMouseDragged:(NSEvent *)event {
	auto mouse = [self transformMouse:event];

	eventDispatcher->app->main.runOnMainThread(
		true, [self, mouse]() { eventDispatcher->touchMoved(mouse.x, mouse.y, MiddleMouseButton); });
	NSEventDispatcher::instance().dispatch(event, self);
}

/// ------------------------

- (void)scrollWheel:(NSEvent *)event { // Mouse scroll wheel movement
	auto mouse = [self transformMouse:event];
	float dx   = event.deltaX;
	float dy   = event.deltaY;
	eventDispatcher->app->main.runOnMainThread(
		true, [self, mouse, dx, dy]() { eventDispatcher->mouseScrolled(mouse.x, mouse.y, dx, dy); });
	NSEventDispatcher::instance().dispatch(event, self);
}

- (void)swipeWithEvent:(NSEvent *)event { // Trackpad swipe gesture
	NSEventDispatcher::instance().dispatch(event, self);
}

- (void)rotateWithEvent:(NSEvent *)event { // Trackpad twist gesture
	NSEventDispatcher::instance().dispatch(event, self);
}

- (void)magnifyWithEvent:(NSEvent *)event { // Trackpad pinch gesture
	NSEventDispatcher::instance().dispatch(event, self);

	if (event.phase == NSEventPhaseChanged) {
		float zoom			= event.magnification;
		float pixelScale	= eventDispatcher->app->g.pixelScale;
		auto titleBarHeight = event.window.frame.size.height - event.window.contentView.frame.size.height;

		float x = event.locationInWindow.x * pixelScale;
		float y = (event.window.frame.size.height - event.locationInWindow.y - titleBarHeight) * pixelScale;

		auto evtDispatcher = eventDispatcher;

		eventDispatcher->app->main.runOnMainThread(
			true, [x, y, zoom, evtDispatcher]() { evtDispatcher->mouseZoomed(x, y, zoom); });
	}
}

- (void)shutdown {
	eventDispatcher->exit();
	eventDispatcher = nullptr;
	[super shutdown];
}

- (std::shared_ptr<App>)getApp {
	return eventDispatcher->app;
}
- (std::shared_ptr<EventDispatcher>)getEventDispatcher {
	return eventDispatcher;
}

- (NSDragOperation)draggingEntered:(id<NSDraggingInfo>)sender {
	NSPasteboard *pboard = [sender draggingPasteboard];
	NSArray *filenames	 = [pboard propertyListForType:NSFilenamesPboardType];
	std::vector<std::string> paths;
	for (NSString *url in filenames) {
		paths.push_back([url UTF8String]);
	}
	dropped = false;
	if (eventDispatcher->canOpenFiles(paths)) {
		acceptedDraggingSequenceNo = sender.draggingSequenceNumber;
		//		auto titleBarHeight = sender.draggingDestinationWindow.frame.size.height - sender.draggingDestinationWindow.contentView.frame.size.height;
		//		float pixelScale = eventDispatcher->app->g.pixelScale;

		//		float x = sender.draggingLocation.x * pixelScale;
		//		float y = (sender.draggingDestinationWindow.frame.size.height - sender.draggingLocation.y - titleBarHeight)  * pixelScale;
		//		runOnMainThread(true, [self,x, y]() {
		//			eventDispatcher->fileDragBegin(x,y,0);
		//		});

		return NSDragOperationCopy;
	} else {
		return NSDragOperationNone;
	}
}

- (void)draggingExited:(nullable id<NSDraggingInfo>)sender {
	auto titleBarHeight = sender.draggingDestinationWindow.frame.size.height
						  - sender.draggingDestinationWindow.contentView.frame.size.height;
	float pixelScale = eventDispatcher->app->g.pixelScale;

	float x = sender.draggingLocation.x * pixelScale;
	float y = (sender.draggingDestinationWindow.frame.size.height - sender.draggingLocation.y - titleBarHeight)
			  * pixelScale;

	eventDispatcher->app->main.runOnMainThread(true, [self, x, y]() { eventDispatcher->fileDragExited(x, y, 0); });
}

- (NSDragOperation)draggingUpdated:(id<NSDraggingInfo>)sender {
	if (sender.draggingSequenceNumber == acceptedDraggingSequenceNo) {
		auto titleBarHeight = sender.draggingDestinationWindow.frame.size.height
							  - sender.draggingDestinationWindow.contentView.frame.size.height;
		float pixelScale = eventDispatcher->app->g.pixelScale;

		float x = sender.draggingLocation.x * pixelScale;
		float y = (sender.draggingDestinationWindow.frame.size.height - sender.draggingLocation.y - titleBarHeight)
				  * pixelScale;
		auto numItems = sender.numberOfValidItemsForDrop;
		eventDispatcher->app->main.runOnMainThread(
			true, [self, x, y, numItems]() { eventDispatcher->fileDragUpdate(x, y, 0, (int) numItems); });

		return NSDragOperationCopy;
	} else {
		return NSDragOperationNone;
	}
}
- (BOOL)performDragOperation:(id<NSDraggingInfo>)sender {
	NSPasteboard *pboard = [sender draggingPasteboard];
	NSArray *filenames	 = [pboard propertyListForType:NSFilenamesPboardType];
	std::vector<ScopedUrlRef> paths;
	for (NSString *url in filenames) {
		paths.push_back(ScopedUrl::create([url UTF8String]));
	}

	//    [self lock];
	eventDispatcher->app->main.runOnMainThreadAndWait([self, paths]() {
		eventDispatcher->filesDropped(paths, 0);
		dropped = true;
	});
	//    [self unlock];

	return true;
}
- (void)draggingEnded:(id<NSDraggingInfo>)sender {
	if (!dropped) {
		eventDispatcher->fileDragCancelled(0);
	}
}

- (BOOL)windowShouldClose:(NSWindow *)sender {
	return YES;
}

- (void)windowDidBecomeKey:(NSNotification *)notification {
	NSWindow *window = notification.object;
	Graphics &g		 = eventDispatcher->app->g;
	g.pixelScale	 = window.screen.backingScaleFactor;
	// Log::e() << "Pixel scale being set for first time: " << g.pixelScale;
}

- (void)windowWillClose:(NSNotification *)notification {
	[[NSApplication sharedApplication] terminate:nil];
}
- (void)windowWillStartLiveResize:(NSNotification *)notification {
	[super disableDrawing];
}
// this helps with crashing when resizing, but you don't get nice resizing
- (void)windowDidEndLiveResize:(NSNotification *)notification {

	[super windowResized:notification];
	[super enableDrawing];
}

@end
