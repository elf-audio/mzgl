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

#ifdef USE_METALANGLE
#	import "MZMGLKViewController.h"
#endif
using namespace std;

@interface MacAppDelegate() {
	id view;

	App *app;
	std::shared_ptr<EventDispatcher> eventDispatcher;
	NSWindow *window;
	
#ifdef USE_METALANGLE
	MZMGLKViewController *controller;
#endif
	
}
@end

@implementation MacAppDelegate

-(id) initWithApp: (void*)_app {
	self = [super init];
	if(self!=nil) {
		app = (App*)_app;
	}
	return self;
}

#ifdef UNIT_TEST
- (App*) getApp {
	return app;
}
- (EventDispatcher*) getEventDispatcher {
	return eventDispatcher;
}
#endif

- (void) makeWebView: (NSString*) url {
    NSRect windowRect = [self setupWindow];
    view = [[MZGLWebView alloc] initWithFrame: windowRect eventDispatcher: eventDispatcher.get() andUrl: url];
    [[window contentView] addSubview:view];
    [window makeKeyAndOrderFront:nil];
    [window makeMainWindow];
	window.delegate = view;
	
	// no runloop because no graphics, so lets make one for runOnMainThread
	NSTimer *timer = [NSTimer timerWithTimeInterval:1.0/60.0 repeats:YES block:^(NSTimer * _Nonnull timer) {
		app->updateInternal();
	}];
	
	[[NSRunLoop mainRunLoop] addTimer:timer forMode:NSDefaultRunLoopMode];
    
}
- (NSRect) setupWindow {
	
	
	
	
	{
	
		NSWindow *___ = [[NSWindow alloc] initWithContentRect:NSMakeRect(0,0,100,100)
													   styleMask:NSTitledWindowMask
														 backing:NSBackingStoreBuffered
														   defer:YES
														  screen:nil];
		

	
	
	}
	
	
	
	
	NSScreen *mainScreen = [NSScreen mainScreen];
	
	
	float w = app->g.width/app->g.pixelScale;
	float h = app->g.height/app->g.pixelScale;

	if(h > mainScreen.visibleFrame.size.height*0.9) {
		h = mainScreen.visibleFrame.size.height*0.9;
		app->g.height = h * app->g.pixelScale;
	}
	
	if(w > mainScreen.visibleFrame.size.width) {
		w = mainScreen.visibleFrame.size.width;
		app->g.width = w * app->g.pixelScale;
	}
	
	   
    NSRect windowRect = NSMakeRect(0, 0, w, h);
    
    window = [[NSWindow alloc]
                 initWithContentRect:windowRect
                 styleMask:NSTitledWindowMask | NSWindowStyleMaskResizable | NSWindowStyleMaskClosable backing:NSBackingStoreBuffered defer:NO];
    window.acceptsMouseMovedEvents = YES;
    id appName = [[NSProcessInfo processInfo] processName];
    
    
    [window cascadeTopLeftFromPoint:NSMakePoint(20,20)];
    [window setTitle:appName];
//	NSLog(@"window frame size: %f x %f, pixelScale: %f", window.frame.size.width, window.frame.size.height, app->g.pixelScale);
	
	// if the screen isn't big enough to create the window, lets update graphics dims here.
//	app->g.width = window.contentView.frame.size.width*app->g.pixelScale;
//	app->g.height = window.contentView.frame.size.height*app->g.pixelScale;
	
    return windowRect;
}

- (void) makeWindow {
	
	
   NSRect windowRect = [self setupWindow];

	
#ifdef USE_METALANGLE
	controller = [[MZMGLKViewController alloc] initWithFrame:windowRect eventDispatcher:eventDispatcher];
	view = controller.view;
#else
	view = [[EventsView alloc] initWithFrame:windowRect eventDispatcher:eventDispatcher.get()];
#endif
	
	
	window.delegate = view;
	[[window contentView] addSubview:view];
	[window makeKeyAndOrderFront:nil];
	[window makeMainWindow];
#ifdef USE_METALANGLE
	app->viewController = controller;
#else
	// this doesn't work - there is no contentViewController
	app->viewController = (__bridge void*)window.windowController.contentViewController;
#endif
	app->windowHandle = (__bridge void*)window;
}

- (void) about:(id) event {
	
	// THIS CRASHES WHEN YOU CLOSE THE WINDOW
	
	NSUInteger windowStyle = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable;
	
	// Window bounds (x, y, width, height).
	NSRect windowRect = NSMakeRect(100, 100, 200, 200);
	NSWindow * win = [[NSWindow alloc] initWithContentRect:windowRect
													styleMask:windowStyle
													  backing:NSBackingStoreBuffered
														defer:NO];
	win.level = NSFloatingWindowLevel;
	NSTextView * textView = [[NSTextView alloc] initWithFrame:windowRect];
	
	[win setContentView:textView];
	id appName = [[NSProcessInfo processInfo] processName];
	[win cascadeTopLeftFromPoint:NSMakePoint(40,40)];
	[textView insertText:[@"About " stringByAppendingString:appName]];
	[win orderFrontRegardless];
}


- (void) makeMenus {
#if 1
	string appName = [[[NSProcessInfo processInfo] processName] UTF8String];
	
	auto appMenu = MacMenuBar::instance().getMenu(appName);
//	appMenu->addItem("About " + appName, "", [self]() {
//		[self about:nil];
//	});
//
//
//	appMenu->addSeparator();
	appMenu->addItem("Quit " + appName, "q", []() {
		[[NSApplication sharedApplication] terminate:nil];
	});
	
#else
	
	id menubar = [NSMenu new];
	id appMenuItem = [NSMenuItem new];
	[menubar addItem:appMenuItem];
	[NSApp setMainMenu:menubar];
	id appMenu = [NSMenu new];
	
	id appName = [[NSProcessInfo processInfo] processName];
	id quitTitle = [@"Quit " stringByAppendingString:appName];
	id quitMenuItem = [[NSMenuItem alloc] initWithTitle:quitTitle
												 action:@selector(terminate:) keyEquivalent:@"q"] ;
	
	id aboutMenuItem = [[NSMenuItem alloc] initWithTitle:[@"About " stringByAppendingString:appName]
												  action:@selector(about:) keyEquivalent:@""];
	[appMenu addItem: aboutMenuItem];
	[appMenu addItem: [NSMenuItem separatorItem]];
	[appMenu addItem:quitMenuItem];
	
	[appMenuItem setSubmenu:appMenu];
#endif
}

- (void)applicationWillFinishLaunching:(NSNotification *)notification {
	Log::d() << "applicationWillFinishLaunching";
}

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification {
	Log::d() << "applicationDidFinishLaunching";
	// Insert code here to initialize your application
	
	eventDispatcher = std::make_shared<EventDispatcher>(app);
	
	if(!app->isHeadless()) {
		[self makeMenus];
        if(app->isWebView()) {
			
			auto *webViewApp = (WebViewApp*)app;
			NSString *url = [NSString stringWithUTF8String:webViewApp->customUrl.c_str()];
            [self makeWebView: url];
            eventDispatcher->setup();
            
            NSRunLoop *rl = [NSRunLoop currentRunLoop];

        } else {
            [self makeWindow];
        }
	}
}

- (void)applicationWillTerminate:(NSNotification *)aNotification {
	// Insert code here to tear down your application
	Log::d() << "applicationWillTerminate";
	[view shutdown];
}
- (BOOL)application:(NSApplication *)sender
		   openFile:(NSString *)filename {
	string fn = [filename UTF8String];
	auto evtDispatcher = eventDispatcher;
	app->main.runOnMainThread(true, [evtDispatcher, fn]() {
		evtDispatcher->openUrl(fn);
	});
	return YES;
}
@end
