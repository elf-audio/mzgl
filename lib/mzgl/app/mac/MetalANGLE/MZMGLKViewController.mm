//
//  MZMGLKViewController.m
//  metalbarebones
//
//  Created by Marek Bereza on 02/02/2021.
//

#import "EventsView.h"

#ifdef USE_METALANGLE
#import "MZMGLKViewController.h"
#import <MetalANGLE/MGLKView.h>
#include <MetalANGLE/GLES3/gl32.h>

@implementation MZMGLKViewController {
	EventsView *glView;
	EventDispatcher *eventDispatcher;
	MGLContext *context;
	NSRect initialFrame;
}

- (id) initWithFrame: (NSRect) frame eventDispatcher:(void*)evtDispatcherPtr {
	self = [super init];
	if(self!=nil) {
		eventDispatcher = (EventDispatcher*) evtDispatcherPtr;
		initialFrame = frame;
	}
	return self;
}

- (void)loadView {
	glView = [[EventsView alloc] initWithFrame:initialFrame eventDispatcher:eventDispatcher];
	
	self.preferredFramesPerSecond = 60;
	glView.drawableDepthFormat = MGLDrawableDepthFormat16;
	glView.drawableMultisample = MGLDrawableMultisample4X;

	context = [[MGLContext alloc] initWithAPI:kMGLRenderingAPIOpenGLES3];
	glView.context = context;
	glView.delegate = self;
	self.view = glView;
	
	[MGLContext setCurrentContext:context];
	glViewport(0, 0, initialFrame.size.width, initialFrame.size.height);
	
}

- (CGSize)size {
	return self.glView.drawableSize;
}


- (void)mglkView:(MGLKView *)view drawInRect:(CGRect)rect {
	// probs don't need this call:
	[MGLContext setCurrentContext:context];
	
	[glView draw];
}

@end
#endif
