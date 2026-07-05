//
//  MZMetaliOSViewController.mm
//  MZGLiOS
//
//  Created by Marek Bereza on 17/01/2018.
//  Copyright © 2018 Marek Bereza. All rights reserved.
//

#include "backendDefines.h"
#ifndef MZGL_OPENGL // Sokol / native Metal backends only

#import "MZMetaliOSViewController.h"
#include "App.h"
#include "EventDispatcher.h"
#include "util.h"
#include "log.h"
#import "MZGLKitView.h"

#ifdef MZGL_PLUGIN
#	include "MZGLEffectAU.h"
#endif

@implementation MZMetaliOSViewController {
	EventsView *mzView;
	BOOL sizeChangeTriggered;
	CGSize lastSize;
}
- (void)deleteCppObjects {
	[mzView deleteCppObjects];
}

- (id)initWithApp:(std::shared_ptr<App>)_app andGraphics:(std::shared_ptr<Graphics>)_graphics {
	self = [super init];
	if (self != nil) {
		mzView	  = [[EventsView alloc] initWithApp:_app andGraphics:_graphics];
		self.view = mzView;
		// MTKView's MSAA sample count is fixed at init, so antialiasing toggling
		// is a no-op on the Metal backend.
		_app->g.setAntialiasing = [](bool a) {};
	}
	return self;
}

// fixes a bug that delays touches on the left edge of the screen
// https://stackoverflow.com/questions/39813245/touchesbeganwithevent-is-delayed-at-left-edge-of-screen
- (void)viewDidAppear:(BOOL)animated {
	[super viewDidAppear:animated];
	if ([self.view.window.gestureRecognizers count] >= 2) {
		UIGestureRecognizer *gr0 = self.view.window.gestureRecognizers[0];
		UIGestureRecognizer *gr1 = self.view.window.gestureRecognizers[1];

		gr0.delaysTouchesBegan = false;
		gr1.delaysTouchesBegan = false;
	}
}

- (void)viewWillTransitionToSize:(CGSize)size
	   withTransitionCoordinator:(id<UIViewControllerTransitionCoordinator>)coordinator {
	[super viewWillTransitionToSize:size withTransitionCoordinator:coordinator];
	sizeChangeTriggered = YES;
	lastSize			= size;
}

- (void)viewDidLayoutSubviews {
	[super viewDidLayoutSubviews];
	if (sizeChangeTriggered) {
		if (mzView != nil) {
			auto app = [mzView getApp];
			if (app != nullptr) {
				app->g.width  = lastSize.width * app->g.pixelScale;
				app->g.height = lastSize.height * app->g.pixelScale;

				auto eventDispatcher = [mzView getEventDispatcher];
				if (eventDispatcher && eventDispatcher->hasSetup()) {
					eventDispatcher->resized();
				}
			}
		}
		sizeChangeTriggered = NO;
	}
}

- (std::shared_ptr<EventDispatcher>)getEventDispatcher {
	return [mzView getEventDispatcher];
}

- (EventsView *)getView {
	return mzView;
}
- (void)dealloc {
	NSLog(@"dealloc MZMetaliOSViewController");
}
@end

#endif // !MZGL_OPENGL
