//
//  MZGLES3Renderer.m
//  MZGLiOS
//
//  Created by Marek Bereza on 17/01/2018.
//  Copyright © 2018 Marek Bereza. All rights reserved.
//

#import "MZGLKitViewController.h"
#include "App.h"
#include "EventDispatcher.h"
#include "util.h"
#include "log.h"
#import "MZGLKitView.h"

#ifdef MZGL_PLUGIN
#	include "MZGLEffectAU.h"
#endif
@implementation MZGLKitViewController {
	MZGLKitView *mzView;
	BOOL currentlyPaused;
}
- (void) deleteCppObjects {
	[mzView deleteCppObjects];
}
// in an AUV3, all instances of the plugin run
// in the same process, maybe even on the same
// thread. So we're going to share the context
//    for now, as switching contexts for every
//  draw call is quite a intensive thing to do
//     apparently. Bit of a hack but it works.

EAGLContext *context = nil;
- (id)initWithApp:(std::shared_ptr<App>)_app {
	self = [super init];
	if (self != nil) {
		currentlyPaused				  = YES;
		self.delegate				  = self;
		self.preferredFramesPerSecond = 60.f;
		mzView						  = [[MZGLKitView alloc] initWithApp:_app];
		self.view					  = mzView;
		GLKView *v					  = self.view;

		_app->g.setAntialiasing = [v](bool a) {
			if (a) {
				v.drawableMultisample = GLKViewDrawableMultisample4X;
			} else {
				v.drawableMultisample = GLKViewDrawableMultisampleNone;
			}
		};

		if (context == nil) {
			context = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES3];
		}

		//		EAGLContext *context = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES3];

		v.drawableColorFormat = GLKViewDrawableColorFormatRGBA8888;
		//view.drawableDepthFormat = GLKViewDrawableDepthFormat24;
		//view.drawableStencilFormat = GLKViewDrawableStencilFormat8;

		// Enable multisampling
		v.drawableMultisample = GLKViewDrawableMultisample4X;
		//		v.drawableMultisample = GLKViewDrawableMultisampleNone;
		v.context = context;
		[EAGLContext setCurrentContext:v.context];
	}
	return self;
}

// this fixes a bug that delays touches on the left hand side of the screen
// https://stackoverflow.com/questions/39813245/touchesbeganwithevent-is-delayed-at-left-edge-of-screen
- (void)viewDidAppear:(BOOL)animated {
	if ([self.view.window.gestureRecognizers count] >= 2) {
		UIGestureRecognizer *gr0 = self.view.window.gestureRecognizers[0];
		UIGestureRecognizer *gr1 = self.view.window.gestureRecognizers[1];

		gr0.delaysTouchesBegan = false;
		gr1.delaysTouchesBegan = false;
	}
}

- (void)viewWillTransitionToSize:(CGSize)size withTransitionCoordinator:(id)coordinator {
	//	NSLog(@"Orientation changed %.0f %.0f", size.width, size.height);
	if (mzView != nil) {
		auto app = [mzView getApp];
		if (app != nullptr) {
			app->g.width  = size.width * app->g.pixelScale;
			app->g.height = size.height * app->g.pixelScale;

			auto eventDispatcher = [mzView getEventDispatcher];
			if (eventDispatcher && eventDispatcher->hasSetup()) {
				eventDispatcher->resized();
			}
		}
	}
}

- (std::shared_ptr<EventDispatcher>)getEventDispatcher {
	return [mzView getEventDispatcher];
}

- (MZGLKitView *)getView {
	return mzView;
}
- (void)dealloc {
	NSLog(@"dealloc MZGLKitViewController");
}

- (void)glkViewController:(GLKViewController *)controller willPause:(BOOL)pause {
	if (pause == currentlyPaused) return;
	[self getEventDispatcher]->iosViewWillPause(pause);
	currentlyPaused = pause;
}

- (void)glkViewControllerUpdate:(nonnull GLKViewController *)controller {
	// do nothing but required
}

@end
