//
//  MZOpenGLView.m
//  MZGL
//
//  Created by Marek Bereza on 15/01/2018.
//  Copyright © 2018 Marek Bereza. All rights reserved.
//

#import "MZGLView.h"
#include "App.h"
#include "util.h"
#include "log.h"

#include "mzOpenGL.h"
#include "Graphics.h"
#include "EventDispatcher.h"
#include "NSEventDispatcher.h"
#include "PluginEditor.h"
#include "Vbo.h"
#include <mutex>

@implementation MZGLView {
	CVDisplayLinkRef displayLink;
	std::mutex evtMutex;
	bool firstFrame;
}

@synthesize view;

- (id) initWithFrame: (NSRect) frame eventDispatcher:(void*) evtDispatcherPtr {
	eventDispatcher = (EventDispatcher*)evtDispatcherPtr;
	
	NSOpenGLPixelFormatAttribute pixelFormatAttributes[] = {

			NSOpenGLPFAOpenGLProfile,
	#ifdef MZGL_GL2
			NSOpenGLProfileVersionLegacy,
	#else
			NSOpenGLProfileVersion4_1Core,
	#endif
					
			NSOpenGLPFAColorSize    , 24                           ,
			NSOpenGLPFAAlphaSize    , 8                            ,
			NSOpenGLPFADoubleBuffer ,
			NSOpenGLPFADepthSize, 32,
			NSOpenGLPFAAccelerated  ,
			NSOpenGLPFASampleBuffers, 1							   ,
			NSOpenGLPFASamples		, 4                            ,
			NSOpenGLPFAMultisample,
			0
		};
		NSOpenGLPixelFormat *pixelFormat = [[NSOpenGLPixelFormat alloc] initWithAttributes:pixelFormatAttributes];
		
		self = [super initWithFrame:frame pixelFormat: pixelFormat];
		if(self != nil) {
			[self setWantsBestResolutionOpenGLSurface:YES];
			[self createGLResources];
			[self createDisplayLink];
			
			firstFrame = true;
		}
		return self;
}

- (void) shutdown {
	[self lock];
	CVDisplayLinkStop(displayLink);
	displayLink = NULL;
	[self unlock];
	
}

-(void) dealloc {
	if(displayLink!=NULL) {
		CVDisplayLinkStop(displayLink);
		displayLink = NULL;
	}
	[super dealloc];
}

CVReturn displayCallback(CVDisplayLinkRef displayLink, const CVTimeStamp *inNow, const CVTimeStamp *inOutputTime, CVOptionFlags flagsIn, CVOptionFlags *flagsOut, void *displayLinkContext) {
	MZGLView *view = (__bridge MZGLView *)(displayLinkContext);
	[view renderForTime:*inOutputTime];
	return kCVReturnSuccess;
}

- (void)createDisplayLink {
	CGDirectDisplayID displayID = CGMainDisplayID();
	CVReturn error = CVDisplayLinkCreateWithCGDisplay(displayID, &displayLink);
	
	if (kCVReturnSuccess == error) {
		// is CFBRidgingRetain ok?
		CVDisplayLinkSetOutputCallback(displayLink, displayCallback, (__bridge void*)(self));
		CVDisplayLinkStart(displayLink);
	} else {
		NSLog(@"Display Link created with error: %d", error);
		displayLink = NULL;
	}
}

- (void) setWindowSize:(float) x y:(float) y {
	NSRect frame = self.window.frame;
	NSSize newSize = CGSizeMake(x, y);
	
	frame.origin.y -= frame.size.height;
	frame.origin.y += newSize.height;
	frame.size = newSize;
	[self.window setFrame: frame display: YES animate: NO];
}

- (void)createGLResources {
	[[self openGLContext] makeCurrentContext];
}

//- (BOOL)windowShouldClose:(NSWindow *)sender {
//	Log::d() << "windowShouldClose";
//	return YES;
//}
//
//
//- (void)windowDidBecomeKey:(NSNotification *)notification {
//	NSWindow *window = notification.object;
//	Graphics &g = eventDispatcher->app->g;
//
//	g.pixelScale = [window backingScaleFactor];
//}
//
//
//
//- (void)windowWillClose:(NSNotification *)notification {
//	//[self shutdown];
//	Log::d() << "windowWillClose";
//	[[NSApplication sharedApplication] terminate:nil];
//	//EventDispatcher::instance()->exit();
//}

- (void)windowResized:(NSNotification *)notification {
	Log::d() << "windowDidResize";
	NSWindow *window = notification.object;
	//WIDTH = window.frame.size.width;
	//HEIGHT = window.frame.size.height;
	Graphics &g = eventDispatcher->app->g;
	g.width = window.contentLayoutRect.size.width;
	g.height = window.contentLayoutRect.size.height;
	g.pixelScale = [window backingScaleFactor];
//	Log::e() << "Pixel scale: "<< g.pixelScale;
	auto f = self.frame;
	f.size.width = g.width;
	f.size.height = g.height;
	glViewport(0, 0, g.width, g.height);
	self.frame = f;
	g.width *= g.pixelScale;
	g.height *= g.pixelScale;
	evtMutex.lock();
	auto evtDispatcher = eventDispatcher;
	runOnMainThread(true, [evtDispatcher]() {
		evtDispatcher->resized();
	});
	evtMutex.unlock();
}

- (void)renderForTime:(CVTimeStamp)time
{
	[[self openGLContext] makeCurrentContext];
	[self lock];

//	drawFrame(eventDispatcher->app->g, eventDispatcher);
	if(eventDispatcher->app->g.firstFrame) {
		initMZGL(eventDispatcher->app);
		eventDispatcher->setup();
		eventDispatcher->app->g.firstFrame = false;
	}
	eventDispatcher->runFrame();
	[self unlock];
	[[self openGLContext] flushBuffer];
}

- (void) lock {
	evtMutex.lock();
}
- (void) unlock {
	evtMutex.unlock();
}

@end


