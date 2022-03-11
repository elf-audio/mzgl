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


#include "NSEventDispatcher.h"
#include "EventDispatcher.h"

using namespace std;

@implementation EventsView {
	NSInteger acceptedDraggingSequenceNo;
	bool dropped;
}
- (id) initWithFrame: (NSRect) frame eventDispatcher:(void*)evtDispatcherPtr {
	self = [super initWithFrame:frame eventDispatcher:evtDispatcherPtr];
	if(self!=nil) {
		dropped = false;
		[self registerForDraggedTypes:[NSArray arrayWithObject:NSFilenamesPboardType]];
	}
	return self;
}

- (BOOL) acceptsFirstResponder {
	return YES;
}

- (BOOL) becomeFirstResponder {
	return YES;
}

// this is for vst so the window becomes focused and accepts
// the first mouse event when unfocussed.
- (BOOL) acceptsFirstMouse:(NSEvent *)theEvent {
	return YES;
}

- (BOOL) isOpaque {
	return YES;
}


int nsEventToKey(NSEvent *evt) {
	string s = [[evt characters] UTF8String];
	if(s.size()==1) {
		return s[0];
	}
	auto keyCode = [evt keyCode];
	switch(keyCode) {
		case 123: return MZ_KEY_LEFT;
		case 124: return MZ_KEY_RIGHT;
		case 125: return MZ_KEY_DOWN;
		case 126: return MZ_KEY_UP;
	}
	printf("keycode: %u\n", keyCode);
	return keyCode;
	
}
- (void) keyDown: (NSEvent*) event {

	auto keyCode = nsEventToKey(event);
	runOnMainThread(true, [self,keyCode]() {
		eventDispatcher->keyDown(keyCode);
	});
	NSEventDispatcher::instance().dispatch(event, self);
	
}
- (void) keyUp: (NSEvent*) event {
	auto keyCode = nsEventToKey(event);
	runOnMainThread(true, [self,keyCode]() {
		eventDispatcher->keyUp(keyCode);
	});
	NSEventDispatcher::instance().dispatch(event, self);
}



- (void) mouseMoved: (NSEvent*) event {
	auto titleBarHeight = event.window.frame.size.height - event.window.contentView.frame.size.height;
	float pixelScale = eventDispatcher->app->g.pixelScale;
	float x = event.locationInWindow.x * pixelScale;
	float y = (event.window.frame.size.height - event.locationInWindow.y - titleBarHeight)  * pixelScale;
	runOnMainThread(true, [self, x, y]() {
		eventDispatcher->touchOver(x,y);
	});
	NSEventDispatcher::instance().dispatch(event, self);
}
- (void) mouseDown: (NSEvent*) event {
	auto titleBarHeight = event.window.frame.size.height - event.window.contentView.frame.size.height;
	float pixelScale = eventDispatcher->app->g.pixelScale;

	float x = event.locationInWindow.x * pixelScale;
	float y = (event.window.frame.size.height - event.locationInWindow.y - titleBarHeight) * pixelScale;
	int id = (int)[event buttonNumber];
	runOnMainThread(true, [self,x, y, id]() {
		eventDispatcher->touchDown(x, y,id);
	});
	NSEventDispatcher::instance().dispatch(event, self);

}

- (void) mouseUp: (NSEvent*) event {
	auto titleBarHeight = event.window.frame.size.height - event.window.contentView.frame.size.height;
	float pixelScale = eventDispatcher->app->g.pixelScale;

	float x = event.locationInWindow.x * pixelScale;
	float y = (event.window.frame.size.height - event.locationInWindow.y - titleBarHeight) * pixelScale;
	int id = (int)[event buttonNumber];
	runOnMainThread(true, [self,x, y, id]() {
		eventDispatcher->touchUp(x, y,id);
	});
	NSEventDispatcher::instance().dispatch(event, self);
}

- (void) mouseDragged: (NSEvent*) event { // Mouse click-and-drag
	auto titleBarHeight = event.window.frame.size.height - event.window.contentView.frame.size.height;
	float pixelScale = eventDispatcher->app->g.pixelScale;

	float x = event.locationInWindow.x * pixelScale;
	float y = (event.window.frame.size.height - event.locationInWindow.y - titleBarHeight) * pixelScale;
	int id = (int)[event buttonNumber];
	runOnMainThread(true, [self,x, y, id]() {
		eventDispatcher->touchMoved(x, y,id);
	});
	NSEventDispatcher::instance().dispatch(event, self);
}

- (void) scrollWheel: (NSEvent*) event { // Mouse scroll wheel movement
	auto titleBarHeight = event.window.frame.size.height - event.window.contentView.frame.size.height;
	float pixelScale = eventDispatcher->app->g.pixelScale;

	float x = event.locationInWindow.x * pixelScale;
	float y = (event.window.frame.size.height - event.locationInWindow.y - titleBarHeight) * pixelScale;
	float dx = event.deltaX;
	float dy = event.deltaY;
	runOnMainThread(true, [self, x, y, dx, dy]() {
		eventDispatcher->mouseScrolled(x,y, dx, dy);
	});
	NSEventDispatcher::instance().dispatch(event, self);
}


- (void) swipeWithEvent: (NSEvent*) event { // Trackpad swipe gesture
	NSEventDispatcher::instance().dispatch(event, self);
}

- (void) rotateWithEvent: (NSEvent*) event { // Trackpad twist gesture
	NSEventDispatcher::instance().dispatch(event, self);
}

- (void) magnifyWithEvent: (NSEvent*) event { // Trackpad pinch gesture
	NSEventDispatcher::instance().dispatch(event, self);

	if(event.phase==NSEventPhaseChanged) {
		float zoom = event.magnification;
		float pixelScale = eventDispatcher->app->g.pixelScale;
		auto titleBarHeight = event.window.frame.size.height - event.window.contentView.frame.size.height;

		float x = event.locationInWindow.x * pixelScale;
		float y = (event.window.frame.size.height - event.locationInWindow.y - titleBarHeight) * pixelScale;

		auto evtDispatcher = eventDispatcher;

		runOnMainThread(true, [x, y, zoom, evtDispatcher]() {
			evtDispatcher->mouseZoomed(x, y, zoom);
		});
	}
}

- (void) shutdown {
	[super shutdown];
	eventDispatcher->exit();
}

- (void*) getApp {
	return eventDispatcher->app;
}

- (NSDragOperation)draggingEntered:(id < NSDraggingInfo >)sender {
	

	NSPasteboard *pboard = [sender draggingPasteboard];
	NSArray *filenames = [pboard propertyListForType:NSFilenamesPboardType];
	vector<string> paths;
	for(NSString *url in filenames) {
		paths.push_back([url UTF8String]);
	}
	dropped = false;
	if(eventDispatcher->canOpenFiles(paths)) {
		acceptedDraggingSequenceNo = sender.draggingSequenceNumber;
		auto titleBarHeight = sender.draggingDestinationWindow.frame.size.height - sender.draggingDestinationWindow.contentView.frame.size.height;
		float pixelScale = eventDispatcher->app->g.pixelScale;

		float x = sender.draggingLocation.x * pixelScale;
		float y = (sender.draggingDestinationWindow.frame.size.height - sender.draggingLocation.y - titleBarHeight)  * pixelScale;
//		runOnMainThread(true, [self,x, y]() {
//			eventDispatcher->fileDragBegin(x,y,0);
//		});
		
		return NSDragOperationCopy;
	} else {
		return NSDragOperationNone;
	}
}

- (NSDragOperation)draggingUpdated:(id<NSDraggingInfo>)sender {
	
	if(sender.draggingSequenceNumber==acceptedDraggingSequenceNo) {
		auto titleBarHeight = sender.draggingDestinationWindow.frame.size.height - sender.draggingDestinationWindow.contentView.frame.size.height;
		float pixelScale = eventDispatcher->app->g.pixelScale;

		
		float x = sender.draggingLocation.x * pixelScale;
		float y = (sender.draggingDestinationWindow.frame.size.height - sender.draggingLocation.y - titleBarHeight)  * pixelScale;
        int numItems = sender.numberOfValidItemsForDrop;
		runOnMainThread(true, [self,x, y, numItems]() {
			eventDispatcher->fileDragUpdate(x, y, 0, numItems);
	   });

		return NSDragOperationCopy;
	} else {
		return NSDragOperationNone;
	}
}


- (BOOL)performDragOperation:(id<NSDraggingInfo>)sender {
	
	NSPasteboard *pboard = [sender draggingPasteboard];
	NSArray *filenames = [pboard propertyListForType:NSFilenamesPboardType];
	vector<string> paths;
	for(NSString *url in filenames) {
		paths.push_back([url UTF8String]);
	}
	
    
//    [self lock];
    runOnMainThreadAndWait([self, paths]() {
        
        eventDispatcher->filesDropped(paths, 0, []() {});
        dropped = true;
        
    });
//    [self unlock];
	
	return true;
}
- (void)draggingEnded:(id<NSDraggingInfo>)sender {
	if(!dropped) {
		eventDispatcher->fileDragCancelled(0);
	}
}

- (BOOL)windowShouldClose:(NSWindow *)sender {
	return YES;
}

- (void)windowDidBecomeKey:(NSNotification *)notification {
	NSWindow *window = notification.object;
	Graphics &g = eventDispatcher->app->g;
	g.pixelScale = [window backingScaleFactor];
	Log::e() << "Pixel scale being set for first time: " << g.pixelScale;
}

- (void)windowWillClose:(NSNotification *)notification {
	[[NSApplication sharedApplication] terminate:nil];
}

- (void)windowDidResize:(NSNotification *)notification {
	[super windowResized: notification];
}



@end
