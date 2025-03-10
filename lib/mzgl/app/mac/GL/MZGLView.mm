//
//  MZOpenGLView.m
//  MZGL
//
//  Created by Marek Bereza on 15/01/2018.
//  Copyright © 2018 Marek Bereza. All rights reserved.
//

#import "MZGLView.h"
#include "App.h"
#include "mainThread.h"
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
	bool drawing;
}

@synthesize view;

- (id)initWithFrame:(NSRect)frame eventDispatcher:(std::shared_ptr<EventDispatcher>)evtDispatcher {
	eventDispatcher = evtDispatcher;

	NSOpenGLPixelFormatAttribute pixelFormatAttributes[] = {

		NSOpenGLPFAOpenGLProfile,

		NSOpenGLProfileVersion3_2Core,

		NSOpenGLPFAColorSize,
		24,
		NSOpenGLPFAAlphaSize,
		8,
		NSOpenGLPFADoubleBuffer,
		NSOpenGLPFADepthSize,
		32,
		NSOpenGLPFAAccelerated,
		NSOpenGLPFASampleBuffers,
		1,
		NSOpenGLPFASamples,
		4,
		NSOpenGLPFAMultisample,
		0};
	NSOpenGLPixelFormat *pixelFormat = [[NSOpenGLPixelFormat alloc] initWithAttributes:pixelFormatAttributes];

	self = [super initWithFrame:frame pixelFormat:pixelFormat];
	if (self != nil) {
		[self setWantsBestResolutionOpenGLSurface:YES];
		[self createGLResources];
		[self createDisplayLink];
		drawing = true;

		firstFrame = true;
	}
	return self;
}

- (void)shutdown {
	[self lock]; // - marek commented this out on 15/07/22 - may cause problems
	CVDisplayLinkStop(displayLink);
	displayLink = NULL;
	[self unlock];
}

- (std::shared_ptr<App>)getApp {
	return eventDispatcher->app;
}
- (std::shared_ptr<EventDispatcher>)getEventDispatcher {
	return eventDispatcher;
}

- (void)dealloc {
	if (displayLink != NULL) {
		CVDisplayLinkStop(displayLink);
		displayLink = NULL;
	}
	// can't do this apparently, but clang warns about it.
	//	[super dealloc];
}

CVReturn displayCallback(CVDisplayLinkRef displayLink,
						 const CVTimeStamp *inNow,
						 const CVTimeStamp *inOutputTime,
						 CVOptionFlags flagsIn,
						 CVOptionFlags *flagsOut,
						 void *displayLinkContext) {
	MZGLView *view = (__bridge MZGLView *) (displayLinkContext);

	if (view->drawing) {
		[view renderForTime:*inOutputTime];
	}
	return kCVReturnSuccess;
}

- (void)createDisplayLink {
	CGDirectDisplayID displayID = CGMainDisplayID();
	CVReturn error				= CVDisplayLinkCreateWithCGDisplay(displayID, &displayLink);

	if (kCVReturnSuccess == error) {
		// is CFBRidgingRetain ok?
		CVDisplayLinkSetOutputCallback(displayLink, displayCallback, (__bridge void *) (self));
		CVDisplayLinkStart(displayLink);
	} else {
		NSLog(@"Display Link created with error: %d", error);
		displayLink = NULL;
	}
}

- (void)setWindowSize:(float)x y:(float)y {
	NSRect frame   = self.window.frame;
	NSSize newSize = CGSizeMake(x, y);

	frame.origin.y -= frame.size.height;
	frame.origin.y += newSize.height;
	frame.size = newSize;
	[self.window setFrame:frame display:YES animate:NO];
}

- (void)createGLResources {
	[[self openGLContext] makeCurrentContext];
}

- (void)windowResized:(NSNotification *)notification {
	Log::d() << "windowDidResize";

	NSWindow *window = notification.object;

	Graphics &g	 = eventDispatcher->app->g;
	g.width		 = window.contentLayoutRect.size.width;
	g.height	 = window.contentLayoutRect.size.height;
	g.pixelScale = [window backingScaleFactor];

	auto f		  = self.frame;
	f.size.width  = g.width;
	f.size.height = g.height;

	self.frame = f;

	glViewport(0, 0, g.width * g.pixelScale, g.height * g.pixelScale);

	g.width *= 2;
	g.height *= 2;

	eventDispatcher->app->main.runOnMainThread(true,
											   [evtDispatcher = eventDispatcher]() { evtDispatcher->resized(); });
}

- (void)renderForTime:(CVTimeStamp)time {
	[[self openGLContext] makeCurrentContext];
	[self lock];

	if (eventDispatcher->app->g.firstFrame) {
		initMZGL(eventDispatcher->app);
		eventDispatcher->setup();
		eventDispatcher->app->g.firstFrame = false;
	}

	eventDispatcher->runFrame();
	[self unlock];
	[[self openGLContext] flushBuffer];
}

- (void)lock {
	evtMutex.lock();
}
- (void)unlock {
	evtMutex.unlock();
}

- (void)disableDrawing {
	drawing = NO;
}

- (void)enableDrawing {
	drawing = YES;
}
@end
