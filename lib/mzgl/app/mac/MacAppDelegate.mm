//
//  AppDelegate.m
//  MZGL
//
//  Created by Marek Bereza on 15/01/2018.
//  Copyright © 2018 Marek Bereza. All rights reserved.
//

#import "MacAppDelegate.h"
#import "EventsView.h"
#include "mainThread.h"
#include "MacMenuBar.h"
#include "App.h"
#include "Graphics.h"
#include "EventDispatcher.h"
#include "log.h"
#include "EventsView.h"
#include "MZGLWebView.h"
#include "util.h"

using namespace std;

#if defined(__APPLE__)
void quitApplication() {
	[NSApp terminate:nil];
}
#endif

void handleTerminateSignal(int signal) {
	Log::d() << "Got a terminate signal";
	dispatch_async(dispatch_get_main_queue(), ^{
	  Log::d() << "Terminating the app";
	  quitApplication();
	});
}

@interface MacAppDelegate () {
	id view;

	std::shared_ptr<App> app;
	std::shared_ptr<EventDispatcher> eventDispatcher;
	NSWindow *window;
}
@end

@implementation MacAppDelegate

- (id)initWithApp:(std::shared_ptr<App>)_app {
	self = [super init];
	if (self != nil) {
		app = _app;
	}
	return self;
}

#ifdef UNIT_TEST
- (std::shared_ptr<App>)getApp {
	return app;
}
- (std::shared_ptr<EventDispatcher>)getEventDispatcher {
	return eventDispatcher;
}
#endif

- (void)makeWebView:(NSString *)url {
	NSRect windowRect = [self setupWindow];
	view			  = [[MZGLWebView alloc] initWithFrame:windowRect eventDispatcher:eventDispatcher andUrl:url];
	[[window contentView] addSubview:view];
	[window makeKeyAndOrderFront:nil];
	[window makeMainWindow];
	window.delegate = view;

	// no runloop because no graphics, so lets make one for runOnMainThread
	NSTimer *timer = [NSTimer timerWithTimeInterval:1.0 / 60.0
											repeats:YES
											  block:^(NSTimer *_Nonnull timer) { app->updateInternal(); }];

	[[NSRunLoop mainRunLoop] addTimer:timer forMode:NSDefaultRunLoopMode];
}

int getTitleBarHeight(NSWindow *window) {
	NSRect frameRect   = window.frame;
	NSRect contentRect = [window contentRectForFrameRect:frameRect];

	return frameRect.size.height - contentRect.size.height;
}
- (NSRect)setupWindow {
	NSScreen *mainScreen = [NSScreen mainScreen];

	float w = app->g.width / app->g.pixelScale;
	float h = app->g.height / app->g.pixelScale;

	if (h > mainScreen.visibleFrame.size.height * 0.9) {
		h			  = mainScreen.visibleFrame.size.height * 0.9;
		app->g.height = h * app->g.pixelScale;
	}

	if (w > mainScreen.visibleFrame.size.width) {
		w			 = mainScreen.visibleFrame.size.width;
		app->g.width = w * app->g.pixelScale;
	}

	NSRect windowRect = NSMakeRect(0, 0, w, h);

	window = [[NSWindow alloc]
		initWithContentRect:windowRect
				  styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskResizable | NSWindowStyleMaskClosable
					backing:NSBackingStoreBuffered
					  defer:NO];

	bool hasTransparentTitleBar = false;
	if (hasTransparentTitleBar) {
		// START OF TRANSPARENT TITLE BAR WINDOW
		CGFloat titleBarHeight = getTitleBarHeight(window);
		windowRect.origin.y -= titleBarHeight;
		windowRect.size.height += titleBarHeight;
		window.titlebarAppearsTransparent = YES;
		window.styleMask |= NSWindowStyleMaskFullSizeContentView;
	}

	window.acceptsMouseMovedEvents = YES;
	id appName					   = [[NSProcessInfo processInfo] processName];

	[window cascadeTopLeftFromPoint:NSMakePoint(20, 20)];
	[window setTitle:appName];

	return windowRect;
}

- (void)makeWindow {
	NSRect windowRect = [self setupWindow];

	view = [[EventsView alloc] initWithFrame:windowRect eventDispatcher:eventDispatcher];

	window.delegate = view;
	[[window contentView] addSubview:view];
	[window makeKeyAndOrderFront:nil];
	[window center];
	[window makeMainWindow];

	// this doesn't work - there is no contentViewController
	app->viewController = (__bridge void *) window.windowController.contentViewController;

	app->windowHandle = (__bridge void *) window;
	app->viewHandle	  = (__bridge void *) view;

	[[NSNotificationCenter defaultCenter] addObserver:self
											 selector:@selector(screenDidChange:)
												 name:NSWindowDidChangeScreenNotification
											   object:window];
}

- (void)about:(id)event {
	// THIS CRASHES WHEN YOU CLOSE THE WINDOW

	NSUInteger windowStyle = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable;

	// Window bounds (x, y, width, height).
	NSRect windowRect	 = NSMakeRect(100, 100, 200, 200);
	NSWindow *win		 = [[NSWindow alloc] initWithContentRect:windowRect
												   styleMask:windowStyle
													 backing:NSBackingStoreBuffered
													   defer:NO];
	win.level			 = NSFloatingWindowLevel;
	NSTextView *textView = [[NSTextView alloc] initWithFrame:windowRect];

	[win setContentView:textView];
	id appName = [[NSProcessInfo processInfo] processName];
	[win cascadeTopLeftFromPoint:NSMakePoint(40, 40)];
	[textView insertText:[@"About " stringByAppendingString:appName]];
	[win orderFrontRegardless];
}

- (void)makeMenus {
	string appName = [[[NSProcessInfo processInfo] processName] UTF8String];

	auto appMenu = MacMenuBar::instance().getMenu(appName);
	appMenu->addItem("Quit " + appName, "q", []() { [[NSApplication sharedApplication] terminate:nil]; });
}

- (void)applicationWillFinishLaunching:(NSNotification *)notification {
	Log::d() << "applicationWillFinishLaunching";
}

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification {
	Log::d() << "applicationDidFinishLaunching";

	signal(SIGTERM, handleTerminateSignal);

	eventDispatcher = std::make_shared<EventDispatcher>(app);

	if (!app->isHeadless()) {
		[self makeMenus];
		if (app->isWebView()) {
			auto webViewApp = dynamic_pointer_cast<WebViewApp>(app);
			NSString *url	= [NSString stringWithUTF8String:webViewApp->customUrl.c_str()];
			[self makeWebView:url];
			eventDispatcher->setup();

			NSRunLoop *rl = [NSRunLoop currentRunLoop];

		} else {
			[self makeWindow];
		}
	}
}
- (void)application:(NSApplication *)application openURLs:(NSArray<NSURL *> *)urls {
	if ([urls count] > 0) {
		eventDispatcher->openUrl(ScopedUrl::create([[urls[0] absoluteString] UTF8String]));
	}
}

- (void)screenDidChange:(NSNotification *)notification {
	//	NSWindow *window = notification.object;
	CGFloat scale = window.screen.backingScaleFactor;
	//	NSLog(@"Window moved to screen with scale: %f", scale);
	app->g.pixelScale = scale;
}

- (void)applicationWillTerminate:(NSNotification *)aNotification {
	[[NSNotificationCenter defaultCenter] removeObserver:self];

	[view shutdown];
	eventDispatcher = nullptr;
	app				= nullptr;
	Log::d() << "applicationWillTerminate " << app.use_count();
}

- (BOOL)application:(NSApplication *)sender openFile:(NSString *)filename {
	string fn		   = [filename UTF8String];
	auto evtDispatcher = eventDispatcher;
	app->main.runOnMainThread(true, [evtDispatcher, fn]() {
		if (evtDispatcher) evtDispatcher->openUrl(ScopedUrl::create(fn));
	});
	return YES;
}

@end
