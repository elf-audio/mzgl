//
//  AudioUnitViewController.m
//  MZGLiOSEffectAU
//
//  Created by Marek Bereza on 05/04/2019.
//  Copyright © 2019 Marek Bereza. All rights reserved.
//

#if !TARGET_OS_SIMULATOR
#	import "AudioUnitViewController.h"
#	import "MZGLEffectAU.h"
#	include <thread>

#	if MZGL_IOS
#		import "MZMetaliOSViewController.h"
#		import "MZGLKitView.h"
#	else
#		import "EventsView.h"
#	endif
#	include "Plugin.h"
#	include "PluginEditor.h"
#	include "EventDispatcher.h"

@interface AudioUnitViewController ()

@end

using namespace std;

@implementation AudioUnitViewController {
#	if MZGL_IOS
	MZRootViewController *vc;
	EventsView *glView;
#	else

	EventsView *glView;
	std::shared_ptr<EventDispatcher> eventDispatcher;
#	endif
	MZGLEffectAU *audioUnit;

	std::shared_ptr<Plugin> plugin;
	std::shared_ptr<PluginEditor> app;
	std::shared_ptr<Graphics> g;
}

- (id)init {
	self = [super init];
	if (self != nil) {
		[self setup];
	}
	return self;
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wobjc-missing-super-calls"
- (void)dealloc {
	NSLog(@"dealloc AudioUnitViewController");
#	if !MZGL_IOS
	glView.paused	= YES;
	glView.delegate = nil;
	[glView removeFromSuperview];
	glView = nil;
	eventDispatcher.reset();
#	endif
	app.reset();
	plugin.reset();
	g.reset();
}
#pragma clang diagnostic pop

- (void)setup {
	app		  = nullptr;
	plugin	  = nullptr;
	audioUnit = nil;
	g		  = nullptr;

 	[self setPreferredContentSize:CGSizeMake(500, 800)];
}
#	if !MZGL_IOS

- (instancetype)initWithNibName:(NSNibName)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil {
	self = [super initWithNibName:nibNameOrNil bundle:[NSBundle bundleForClass:self.class]];
	if (self != nil) {
		[self setup];
	}
	return self;
}
#	endif

#	if MZGL_IOS
- (EventsView *)getView {
	return glView;
}
#	else
- (EventsView *)getView {
	return glView;
}
#	endif

- (void)didReceiveMemoryWarning {
	auto ed = [glView getEventDispatcher];
	if (ed != nullptr && ed->hasSetup()) {
		ed->memoryWarning();
	}
}

- (void)viewDidDisappear {
	[[self.view subviews] makeObjectsPerformSelector:@selector(removeFromSuperview)];
}

- (void)addGLView {
	// strip out all subviews first
	[[self.view subviews] makeObjectsPerformSelector:@selector(removeFromSuperview)];
	[self.view addSubview:glView];
}

- (void)viewDidLayoutSubviews {
	[self tryToResize];
}

#	if !MZGL_IOS
- (CGFloat)currentBackingScale {
	NSWindow *window = self.view.window;
	if (window != nil) {
		return window.backingScaleFactor;
	}
	NSScreen *screen = NSScreen.mainScreen;
	if (screen != nil) {
		return screen.backingScaleFactor;
	}
	return 1.0;
}

- (void)viewDidLayout {
	[super viewDidLayout];
	if (glView == nil || app == nullptr) {
		return;
	}
	glView.frame  = self.view.bounds;
	CGFloat scale = [self currentBackingScale];
	g->pixelScale = scale;
	g->width	  = self.view.bounds.size.width * scale;
	g->height	  = self.view.bounds.size.height * scale;
	eventDispatcher->resized();
}
#	endif

- (void)tryToResize {
	if (self.view.window != nil && glView != nil) {
		glView.frame = self.view.frame;
#	if MZGL_IOS
		auto ed = [glView getEventDispatcher];
		if (ed != nullptr && ed->hasSetup()) {
			CLANG_IGNORE_WARNINGS_BEGIN("-Wnonnull")
			[vc viewWillTransitionToSize:self.view.window.frame.size withTransitionCoordinator:nil];
			CLANG_IGNORE_WARNINGS_END
		}
#	endif

	} else {
		NSLog(@"Window null");
	}
}

- (void)viewWillDisappear:(BOOL)animated {
	if (app) app->pluginViewDisappeared();
}

// this is the mac version
// calls the iOS version of viewWillAppear
- (void)viewWillAppear {
	[self viewWillAppear:NO];
}
- (void)viewWillDisappear {
	[self viewWillDisappear:NO];
}
- (void)viewDidAppear {
	[self viewDidAppear:NO];
}

- (void)viewWillAppear:(BOOL)animated {
	if (app == nullptr) {
		g	   = std::make_shared<Graphics>();
		plugin = [self getPlugin];
		app	   = instantiatePluginEditor(*g, plugin);
#	if MZGL_IOS
		vc					= [[MZRootViewController alloc] initWithApp:app andGraphics:g];
		app->viewController = (__bridge void *) self;

		glView = (EventsView *) vc.view;
#	else
		eventDispatcher = std::make_shared<EventDispatcher>(app);
		glView			= [[EventsView alloc] initWithFrame:self.view.frame eventDispatcher:eventDispatcher];
		glView.embeddedInHost	= YES;
		glView.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
#	endif
		glView.frame = self.view.frame;
#	if !MZGL_IOS
		CGFloat scale = [self currentBackingScale];
		g->pixelScale = scale;
		g->width	  = self.view.frame.size.width * scale;
		g->height	  = self.view.frame.size.height * scale;
		eventDispatcher->resized();
#	endif
		[self addGLView];
	}
}

- (void)viewDidAppear:(BOOL)animated {
	NSLog(@"MZGL: viewDidAppear");
	[self addGLView];
	[self tryToResize];
	if (app) app->pluginViewAppeared();
}

- (void)viewWillTransitionToSize:(CGSize)size withTransitionCoordinator:(id)coordinator {
	NSLog(@"MZGL: Size change whaaa");
}

- (std::shared_ptr<Plugin>)getPlugin {
	if (plugin == nullptr) {
		plugin = instantiatePlugin();
	}
	return plugin;
}
- (AUAudioUnit *)createAudioUnitWithComponentDescription:(AudioComponentDescription)desc error:(NSError **)error {
	plugin = [self getPlugin];
	audioUnit = [[MZGLEffectAU alloc] initWithPlugin:plugin andComponentDescription:desc error:error];
	NSLog(@"MZGL: createAudioUnitWithComponentDescription");


	return audioUnit;
}

@end
#endif
