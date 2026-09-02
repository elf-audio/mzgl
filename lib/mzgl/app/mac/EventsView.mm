//
//  EventsView.m
//  mzgl macOS
//
//  Created by Marek Bereza on 03/02/2021.
//  Copyright © 2021 Marek Bereza. All rights reserved.
//

#import "EventsView.h"
#include "util.h"

#include "NSEventDispatcher.h"
#include "EventDispatcher.h"

// the view that most recently received a left-mouse down/drag - this is the
// view a startNativeFileDrag() call refers to (there can be several EventsViews
// alive at once when running as a plugin with multiple editor windows open).
// MRC file so no __weak - cleared in shutdown to avoid dangling.
static EventsView *currentDragSourceView = nil;

// A file promise with a plain file-URL fallback on the same pasteboard item.
// Promise-aware receivers (Finder, Mail, ...) ask us to write the file into a
// destination they choose, so they own their copy; receivers that ignore
// promises (most DAWs) read the URL and get the already-written file as before.
// userInfo holds the source file's absolute path (NSString).
@interface MZGLFilePromiseProvider : NSFilePromiseProvider
@end

@implementation MZGLFilePromiseProvider

- (NSArray<NSPasteboardType> *)writableTypesForPasteboard:(NSPasteboard *)pasteboard {
	NSMutableArray *types = [[[super writableTypesForPasteboard:pasteboard] mutableCopy] autorelease];
	[types addObject:NSPasteboardTypeFileURL];
	return types;
}

- (NSPasteboardWritingOptions)writingOptionsForType:(NSPasteboardType)type
										 pasteboard:(NSPasteboard *)pasteboard {
	if ([type isEqualToString:NSPasteboardTypeFileURL]) return 0;
	return [super writingOptionsForType:type pasteboard:pasteboard];
}

- (id)pasteboardPropertyListForType:(NSPasteboardType)type {
	if ([type isEqualToString:NSPasteboardTypeFileURL]) {
		return [[NSURL fileURLWithPath:(NSString *) self.userInfo] pasteboardPropertyListForType:type];
	}
	return [super pasteboardPropertyListForType:type];
}

@end

static NSString *utiForFileAtPath(NSString *path) {
	static NSDictionary<NSString *, NSString *> *utis = nil;
	if (utis == nil) {
		utis = [@{
			@"wav" : @"com.microsoft.waveform-audio",
			@"aif" : @"public.aiff-audio",
			@"aiff" : @"public.aiff-audio",
			@"mp3" : @"public.mp3",
			@"m4a" : @"com.apple.m4a-audio",
			@"flac" : @"org.xiph.flac",
			@"ogg" : @"org.xiph.ogg-audio",
		} retain];
	}
	NSString *uti = utis[path.pathExtension.lowercaseString];
	return uti != nil ? uti : @"public.data";
}

@interface EventsView () <NSDraggingSource, NSFilePromiseProviderDelegate>
@end

@implementation EventsView {
	NSInteger acceptedDraggingSequenceNo;
	NSEvent *lastLeftMouseEvent;
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
		_handlesKeyboard = YES;
		[self registerForDraggedTypes:@[ NSPasteboardTypeFileURL ]];
	}
	return self;
}

- (BOOL)mouseDownCanMoveWindow {
	return !_embeddedInHost;
}

- (BOOL)acceptsFirstResponder {
	return _handlesKeyboard;
}

- (BOOL)becomeFirstResponder {
	return _handlesKeyboard;
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
		default: break;
	}
	printf("keycode: %u\n", keyCode);
	return keyCode;
}

- (void)flagsChanged:(NSEvent *)event {
	if (!_handlesKeyboard) {
		[super flagsChanged:event];
		return;
	}
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
	if (!_handlesKeyboard) {
		[super keyDown:event];
		return;
	}
	// If a text field is focused, route keystrokes to it as text instead of
	// dispatching them as app hotkeys.
	if (eventDispatcher->app->g.textInputReceiver != nullptr) {
		NSString *chars = [event characters];
		std::string s	= chars ? std::string([chars UTF8String]) : std::string();
		unichar first	= [chars length] > 0 ? [chars characterAtIndex:0] : 0;
		eventDispatcher->app->main.runOnMainThread(true, [self, s, first]() {
			Graphics &g = eventDispatcher->app->g;
			if (g.textInputReceiver == nullptr) return;
			if (first == 8 || first == 127) { // backspace / delete
				eventDispatcher->textBackspace();
			} else if (first == 13 || first == 10 || first == 3) { // return / newline / enter
				eventDispatcher->textDone();
			} else if (first == 27) { // escape
				g.hideKeyboard();
			} else if (!s.empty() && (unsigned char) s[0] >= 32) {
				eventDispatcher->textInput(s);
			}
		});
		return;
	}
	auto keyCode = nsEventToKey(event);
	eventDispatcher->app->main.runOnMainThread(true, [self, keyCode]() { eventDispatcher->keyDown(keyCode); });
	NSEventDispatcher::instance().dispatch(event, self);
}
- (void)keyUp:(NSEvent *)event {
	if (!_handlesKeyboard) {
		[super keyUp:event];
		return;
	}
	auto keyCode = nsEventToKey(event);
	eventDispatcher->app->main.runOnMainThread(true, [self, keyCode]() { eventDispatcher->keyUp(keyCode); });
	NSEventDispatcher::instance().dispatch(event, self);
}
- (vec2)pixelPointFromWindowPoint:(NSPoint)windowPoint {
	NSPoint local	 = [self convertPoint:windowPoint fromView:nil];
	float pixelScale = self.window != nil ? self.window.backingScaleFactor : 1.f;
	float x			 = local.x * pixelScale;
	float y			 = (self.bounds.size.height - local.y) * pixelScale;
	return vec2(x, y);
}
- (vec2)transformMouse:(NSEvent *)event {
	return [self pixelPointFromWindowPoint:event.locationInWindow];
}
- (void)mouseMoved:(NSEvent *)event {
	auto mouse = [self transformMouse:event];
	eventDispatcher->app->main.runOnMainThread(true,
											   [self, mouse]() { eventDispatcher->touchOver(mouse.x, mouse.y); });
	NSEventDispatcher::instance().dispatch(event, self);
}

// with a transparent titlebar + full-size content view this view extends
// under the titlebar - clicks there should only drag the window, never
// reach the app. (drag/up events with no preceding down are ignored by
// the layer system, so only downs need filtering)
static BOOL eventIsInTitleBar(NSEvent *event) {
	return event.locationInWindow.y > NSMaxY(event.window.contentLayoutRect);
}

/// ----------------
- (void)mouseDown:(NSEvent *)event {
	if (!_embeddedInHost && eventIsInTitleBar(event)) return;
	[lastLeftMouseEvent release];
	lastLeftMouseEvent	  = [event retain];
	currentDragSourceView = self;
	auto mouse = [self transformMouse:event];
	eventDispatcher->app->main.runOnMainThread(
		true, [self, mouse]() { eventDispatcher->touchDown(mouse.x, mouse.y, 0); });
	NSEventDispatcher::instance().dispatch(event, self);
}
- (void)rightMouseDown:(NSEvent *)event {
	if (!_embeddedInHost && eventIsInTitleBar(event)) return;
	auto mouse = [self transformMouse:event];
	eventDispatcher->app->main.runOnMainThread(
		true, [self, mouse]() { eventDispatcher->touchDown(mouse.x, mouse.y, RightMouseButton); });
	NSEventDispatcher::instance().dispatch(event, self);
}

- (void)otherMouseDown:(NSEvent *)event {
	if (!_embeddedInHost && eventIsInTitleBar(event)) return;
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
	[lastLeftMouseEvent release];
	lastLeftMouseEvent	  = [event retain];
	currentDragSourceView = self;
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
		float zoom = event.magnification;
		auto p	   = [self pixelPointFromWindowPoint:event.locationInWindow];

		auto evtDispatcher = eventDispatcher;

		eventDispatcher->app->main.runOnMainThread(
			true, [p, zoom, evtDispatcher]() { evtDispatcher->mouseZoomed(p.x, p.y, zoom); });
	}
}

- (void)shutdown {
	if (currentDragSourceView == self) currentDragSourceView = nil;
	[lastLeftMouseEvent release];
	lastLeftMouseEvent = nil;
	// exit() is app-quit semantics (KoalaApp::exit tears down the audio system,
	// clears undo, removes temp files...). When embedded in a plugin host the
	// host owns the processor lifetime and keeps calling process() after the
	// editor window closes, so only the standalone app may exit here.
	if (!_embeddedInHost) eventDispatcher->exit();
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
	// a drag that originated in this app is our own drag-out re-entering the
	// window - reject it so it doesn't get treated as an external file drop
	if ([sender draggingSource] != nil) return NSDragOperationNone;
	NSPasteboard *pboard		= [sender draggingPasteboard];
	NSArray<NSURL *> *filenames = [pboard readObjectsForClasses:@[ [NSURL class] ]
														options:@{NSPasteboardURLReadingFileURLsOnlyKey : @YES}];
	std::vector<std::string> paths;
	for (NSURL *url in filenames) {
		paths.push_back([[url path] UTF8String]);
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
	auto p = [self pixelPointFromWindowPoint:sender.draggingLocation];
	eventDispatcher->app->main.runOnMainThread(true,
											   [self, p]() { eventDispatcher->fileDragExited(p.x, p.y, 0); });
}

- (NSDragOperation)draggingUpdated:(id<NSDraggingInfo>)sender {
	if ([sender draggingSource] != nil) return NSDragOperationNone;
	if (sender.draggingSequenceNumber == acceptedDraggingSequenceNo) {
		auto p		  = [self pixelPointFromWindowPoint:sender.draggingLocation];
		auto numItems = sender.numberOfValidItemsForDrop;
		eventDispatcher->app->main.runOnMainThread(
			true, [self, p, numItems]() { eventDispatcher->fileDragUpdate(p.x, p.y, 0, (int) numItems); });

		return NSDragOperationCopy;
	} else {
		return NSDragOperationNone;
	}
}
- (BOOL)performDragOperation:(id<NSDraggingInfo>)sender {
	if ([sender draggingSource] != nil) return NO;
	NSPasteboard *pboard		= [sender draggingPasteboard];
	NSArray<NSURL *> *filenames = [pboard readObjectsForClasses:@[ [NSURL class] ]
														options:@{NSPasteboardURLReadingFileURLsOnlyKey : @YES}];
	std::vector<ScopedUrlRef> paths;
	for (NSURL *url in filenames) {
		paths.push_back(ScopedUrl::create([[url path] UTF8String]));
	}

	dropped = true;
	eventDispatcher->app->main.runOnMainThread(true, [self, paths]() {
		eventDispatcher->filesDropped(paths, 0);
	});

	return true;
}
- (void)draggingEnded:(id<NSDraggingInfo>)sender {
	if (!dropped) {
		eventDispatcher->fileDragCancelled(0);
	}
}

/// ------------------------ native drag-out (NSDraggingSource)

- (BOOL)startFileDrag:(NSString *)path {
	if (lastLeftMouseEvent == nil) return NO;

	MZGLFilePromiseProvider *provider =
		[[[MZGLFilePromiseProvider alloc] initWithFileType:utiForFileAtPath(path) delegate:self] autorelease];
	provider.userInfo = path;

	NSDraggingItem *item = [[[NSDraggingItem alloc] initWithPasteboardWriter:provider] autorelease];
	NSImage *icon		 = [[NSWorkspace sharedWorkspace] iconForFile:path];

	const CGFloat iconSize = 64;
	NSPoint p			   = [self convertPoint:lastLeftMouseEvent.locationInWindow fromView:nil];
	[item setDraggingFrame:NSMakeRect(p.x - iconSize / 2, p.y - iconSize / 2, iconSize, iconSize)
				  contents:icon];

	NSDraggingSession *session = [self beginDraggingSessionWithItems:@[ item ]
															   event:lastLeftMouseEvent
															  source:self];

	session.animatesToStartingPositionsOnCancelOrFail = YES;
	return YES;
}

- (NSDragOperation)draggingSession:(NSDraggingSession *)session
	sourceOperationMaskForDraggingContext:(NSDraggingContext)context {
	// drag-out is one-way: dropping back inside the app cancels (the image
	// springs back) rather than triggering the app's own file-drop handling
	return context == NSDraggingContextOutsideApplication ? NSDragOperationCopy : NSDragOperationNone;
}

- (void)draggingSession:(NSDraggingSession *)session
		   endedAtPoint:(NSPoint)screenPoint
			  operation:(NSDragOperation)operation {
	// the OS swallowed the mouseUp while it owned the drag - synthesize a
	// touchUp so the layer system doesn't keep a stuck touch focused
	NSRect r = [self.window convertRectFromScreen:NSMakeRect(screenPoint.x, screenPoint.y, 0, 0)];
	auto p	 = [self pixelPointFromWindowPoint:r.origin];
	eventDispatcher->app->main.runOnMainThread(true,
											   [self, p]() { eventDispatcher->touchUp(p.x, p.y, 0); });
}

/// ------------------------ NSFilePromiseProviderDelegate

- (NSString *)filePromiseProvider:(NSFilePromiseProvider *)filePromiseProvider
				  fileNameForType:(NSString *)fileType {
	return [(NSString *) filePromiseProvider.userInfo lastPathComponent];
}

- (void)filePromiseProvider:(NSFilePromiseProvider *)filePromiseProvider
		  writePromiseToURL:(NSURL *)url
		  completionHandler:(void (^)(NSError *errorOrNil))completionHandler {
	NSError *error = nil;
	[[NSFileManager defaultManager] copyItemAtPath:(NSString *) filePromiseProvider.userInfo
											toPath:url.path
											 error:&error];
	completionHandler(error);
}

/// ------------------------

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

// declared in util.h - non-mac platforms get a stub in util.cpp
bool startNativeFileDrag(const std::string &filePath) {
	EventsView *view = currentDragSourceView;
	if (view == nil) return false;
	return [view startFileDrag:[NSString stringWithUTF8String:filePath.c_str()]] == YES;
}
