//
//  AppDelegate.m
//  MZGL
//
//  Created by Marek Bereza on 15/01/2018.
//  Copyright © 2018 Marek Bereza. All rights reserved.
//

#import "MacAppDelegate.h"
#import "EventsView.h"
#include "util.h"
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
	EventDispatcher *eventDispatcher;
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

- (void) makeWebView {
    NSRect windowRect = [self setupWindow];
    view = [[MZGLWebView alloc] initWithFrame: windowRect eventDispatcher: eventDispatcher];
    [[window contentView] addSubview:view];
    [window makeKeyAndOrderFront:nil];
    [window makeMainWindow];
    
}
- (NSRect) setupWindow {
    NSRect windowRect = NSMakeRect(0, 0, app->g.width/app->g.pixelScale, app->g.height/app->g.pixelScale);
    
    window = [[NSWindow alloc]
                 initWithContentRect:windowRect
                 styleMask:NSTitledWindowMask | NSWindowStyleMaskResizable | NSWindowStyleMaskClosable backing:NSBackingStoreBuffered defer:NO];
    window.acceptsMouseMovedEvents = YES;
    
    id appName = [[NSProcessInfo processInfo] processName];
    
    
    [window cascadeTopLeftFromPoint:NSMakePoint(20,20)];
    [window setTitle:appName];
    return windowRect;
}

- (void) makeWindow {
	
	
   NSRect windowRect = [self setupWindow];

	
#ifdef USE_METALANGLE
	controller = [[MZMGLKViewController alloc] initWithFrame:windowRect eventDispatcher:eventDispatcher];
	view = controller.view;
#else
	view = [[EventsView alloc] initWithFrame:windowRect eventDispatcher:eventDispatcher];
#endif
	
	
	window.delegate = view;
	[[window contentView] addSubview:view];
	[window makeKeyAndOrderFront:nil];
	[window makeMainWindow];
}

- (void) about:(id) event {
	NSUInteger windowStyle = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable;
	
	// Window bounds (x, y, width, height).
	NSRect windowRect = NSMakeRect(100, 100, 200, 200);
	NSWindow * window = [[NSWindow alloc] initWithContentRect:windowRect
													styleMask:windowStyle
													  backing:NSBackingStoreBuffered
														defer:NO];
	window.level = NSFloatingWindowLevel;
	NSTextView * textView = [[NSTextView alloc] initWithFrame:windowRect];
	
	[window setContentView:textView];
	id appName = [[NSProcessInfo processInfo] processName];
	[window cascadeTopLeftFromPoint:NSMakePoint(40,40)];
	[textView insertText:[@"About " stringByAppendingString:appName]];
	[window orderFrontRegardless];
}


- (void) makeMenus {
#if 1
	string appName = [[[NSProcessInfo processInfo] processName] UTF8String];
	
	auto appMenu = MacMenuBar::instance().getMenu(appName);
	appMenu->addItem("About " + appName, "", [self]() {
		[self about:nil];
	});
	
	
	appMenu->addSeparator();
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
	
	eventDispatcher = new EventDispatcher(app);
	
	if(!app->isHeadless()) {
		[self makeMenus];
        if(app->isWebView()) {
            [self makeWebView];
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
	runOnMainThread(true, [evtDispatcher, fn]() {
		evtDispatcher->openUrl(fn);
	});
	return YES;
}
@end
