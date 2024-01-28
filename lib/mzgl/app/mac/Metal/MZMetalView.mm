//
//  MZOpenGLView.m
//  MZGL
//
//  Created by Marek Bereza on 15/01/2018.
//  Copyright © 2018 Marek Bereza. All rights reserved.
//

#import "MZMetalView.h"
#include "App.h"
#include "mainThread.h"
#include "log.h"

#include "Graphics.h"
#include "EventDispatcher.h"
#include "NSEventDispatcher.h"
#include "PluginEditor.h"
#include "Vbo.h"
#include <mutex>

@interface MZMetalView ()
@property(nonatomic, strong) id<MTLCommandQueue> commandQueue;
@end

@implementation MZMetalView {
	CVDisplayLinkRef displayLink;
	std::mutex evtMutex;
	bool firstFrame;
	bool drawing;
}

- (id)initWithFrame:(NSRect)frame eventDispatcher:(std::shared_ptr<EventDispatcher>)evtDispatcher {
	eventDispatcher = evtDispatcher;

	// 	NSOpenGLPixelFormatAttribute pixelFormatAttributes[] = {

	// 		NSOpenGLPFAOpenGLProfile,
	// #ifdef MZGL_GL2
	// 		NSOpenGLProfileVersionLegacy,
	// #else
	// 		NSOpenGLProfileVersion4_1Core,
	// #endif

	// 		NSOpenGLPFAColorSize,
	// 		24,
	// 		NSOpenGLPFAAlphaSize,
	// 		8,
	// 		NSOpenGLPFADoubleBuffer,
	// 		NSOpenGLPFADepthSize,
	// 		32,
	// 		NSOpenGLPFAAccelerated,
	// 		NSOpenGLPFASampleBuffers,
	// 		1,
	// 		NSOpenGLPFASamples,
	// 		4,
	// 		NSOpenGLPFAMultisample,
	// 		0};
	// 	NSOpenGLPixelFormat *pixelFormat = [[NSOpenGLPixelFormat alloc] initWithAttributes:pixelFormatAttributes];

	self = [super initWithFrame:frame device:MTLCreateSystemDefaultDevice()];

	if (self != nil) {
		self.delegate = self;

		self.clearColor	  = MTLClearColorMake(0.5, 0., 0.5, 1.0); // Set clear color (gray in this case)
		self.commandQueue = [self.device newCommandQueue];
		// [self setWantsBestResolutionOpenGLSurface:YES];
		// [self createGLResources];
		// [self createDisplayLink];
		drawing = true;

		firstFrame = true;
	}
	return self;
}

- (void)drawInMTKView:(MTKView *)view {
	// Create a new command buffer
	id<MTLCommandBuffer> commandBuffer = [self.commandQueue commandBuffer];

	// Obtain a renderPassDescriptor from the view
	MTLRenderPassDescriptor *renderPassDescriptor = view.currentRenderPassDescriptor;

	if (renderPassDescriptor != nil) {
		// Create a render command encoder
		id<MTLRenderCommandEncoder> renderEncoder =
			[commandBuffer renderCommandEncoderWithDescriptor:renderPassDescriptor];

		eventDispatcher->draw();

		[renderEncoder endEncoding];

		// Present the drawable to the screen
		[commandBuffer presentDrawable:view.currentDrawable];
	}

	// Commit the command buffer
	[commandBuffer commit];
}

- (void)mtkView:(MTKView *)view drawableSizeWillChange:(CGSize)size {
	// Handle view size changes if necessary
}

- (std::shared_ptr<App>)getApp {
	return eventDispatcher->app;
}
- (std::shared_ptr<EventDispatcher>)getEventDispatcher {
	return eventDispatcher;
}

- (void)setWindowSize:(float)x y:(float)y {
	NSRect frame   = self.window.frame;
	NSSize newSize = CGSizeMake(x, y);

	frame.origin.y -= frame.size.height;
	frame.origin.y += newSize.height;
	frame.size = newSize;
	[self.window setFrame:frame display:YES animate:NO];
	//	eventDispatcher->resized();
}

- (void)windowResized:(NSNotification *)notification {
	Log::d() << "windowDidResize";

	NSWindow *window = notification.object;

	Graphics &g	  = eventDispatcher->app->g;
	g.width		  = window.contentLayoutRect.size.width;
	g.height	  = window.contentLayoutRect.size.height;
	g.pixelScale  = [window backingScaleFactor];
	auto f		  = self.frame;
	f.size.width  = g.width;
	f.size.height = g.height;

	//	evtMutex.lock();
	self.frame = f;
	// -need to replace mzmetalview glViewport(0, 0, g.width, g.height);
	g.width *= g.pixelScale;
	g.height *= g.pixelScale;

	auto evtDispatcher = eventDispatcher;
	eventDispatcher->app->main.runOnMainThread(true, [evtDispatcher, &g]() { evtDispatcher->resized(); });
	//	evtMutex.unlock();
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
