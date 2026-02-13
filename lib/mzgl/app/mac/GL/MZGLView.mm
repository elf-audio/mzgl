//
//  MZOpenGLView.m
//  MZGL
//
//  Created by Marek Bereza on 15/01/2018.
//  Copyright © 2018 Marek Bereza. All rights reserved.
//

#import "MZGLView.h"
#import "GLRenderer.h"
#include "App.h"
#include "mainThread.h"
#include "log.h"

#include "mzOpenGL.h"
#include "Graphics.h"
#include "EventDispatcher.h"
#include "NSEventDispatcher.h"
#include "PluginEditor.h"
#include "Vbo.h"

@implementation MZGLView

@synthesize view;

- (id)initWithFrame:(NSRect)frame
	eventDispatcher:(std::shared_ptr<EventDispatcher>)evtDispatcher
	 withRenderMode:(RenderMode)renderMode {
	eventDispatcher = evtDispatcher;

	NSOpenGLPixelFormatAttribute pixelFormatAttributes[] = {NSOpenGLPFAOpenGLProfile,
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

		if (renderMode == RenderMode::UseRenderTimer) {
			renderer = std::make_unique<TimerRenderer>(self);
		} else {
			renderer = std::make_unique<CVDisplayLinkRenderer>(self);
		}
		renderer->start();
	}
	return self;
}

- (id)initWithFrame:(NSRect)frame eventDispatcher:(std::shared_ptr<EventDispatcher>)evtDispatcher {
	return [self initWithFrame:frame eventDispatcher:evtDispatcher withRenderMode:RenderMode::UseCVDisplayLink];
}

- (void)shutdown {
	if (renderer) {
		renderer->stop();
	}
}

- (void)makeContextCurrentForCleanup {
	[[self openGLContext] makeCurrentContext];
}

- (std::shared_ptr<App>)getApp {
	return eventDispatcher->app;
}

- (std::shared_ptr<EventDispatcher>)getEventDispatcher {
	return eventDispatcher;
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wobjc-missing-super-calls"
- (void)dealloc {
	if (renderer) {
		renderer->stop();
		renderer.reset();
	}
}
#pragma clang diagnostic pop

- (void)performRender {
	[[self openGLContext] makeCurrentContext];

	if (!eventDispatcher || !renderer || !renderer->isDrawing()) {
		return;
	}

	if (eventDispatcher->app->g.firstFrame) {
		initMZGL(eventDispatcher->app);
		eventDispatcher->setup();
		eventDispatcher->app->g.firstFrame = false;
	}

	eventDispatcher->runFrame();
	[[self openGLContext] flushBuffer];
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

	Graphics &g = eventDispatcher->app->g;
	g.width		= window.contentLayoutRect.size.width;
	g.height	= window.contentLayoutRect.size.height;

	bool hasTransparentTitleBar = true;

	if (hasTransparentTitleBar) {
		g.width	 = window.frame.size.width;
		g.height = window.frame.size.height;
	}
	g.pixelScale = [window backingScaleFactor];

	auto f		  = self.frame;
	f.size.width  = g.width;
	f.size.height = g.height;
	self.frame	  = f;

	glViewport(0, 0, g.width * g.pixelScale, g.height * g.pixelScale);

	g.width *= 2;
	g.height *= 2;

	eventDispatcher->app->main.runOnMainThread(true,
											   [evtDispatcher = eventDispatcher]() { evtDispatcher->resized(); });
}

- (void)lock {
	if (renderer) {
		renderer->lock();
	}
}

- (void)unlock {
	if (renderer) {
		renderer->unlock();
	}
}

- (void)disableDrawing {
	if (renderer) {
		renderer->setDrawing(false);
	}
}

- (void)enableDrawing {
	if (renderer) {
		renderer->setDrawing(true);
	}
}

@end
