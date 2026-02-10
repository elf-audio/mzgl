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
#		import "MZGLKitViewController.h"
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
	MZGLKitViewController *vc;
	MZGLKitView *glView;
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
	//#if !TARGET_OS_IOS
	//        auto &g = appHolder.g;
	//        appHolder.app = std::shared_ptr<App>(instantiateApp(g));
	//        self.view = [[NSView alloc] initWithFrame: CGRectMake(0, 0, g.width/(float)g.pixelScale, g.height/(float)g.pixelScale)];
	//        [self setPreferredContentSize: CGSizeMake(g.width/(float)g.pixelScale, g.height/(float)g.pixelScale)];
	//        appHolder.eventDispatcher = std::make_shared<EventDispatcher>(appHolder.app);
	//        glView = [[MZGLView alloc] initWithFrame: self.view.frame eventDispatcher: appHolder.eventDispatcher.get()];
	//
	//
	////        [self addGLView];
	////        [self connectViewToAU];
	//#else
	[self setPreferredContentSize:CGSizeMake(500, 800)];
	//#endif
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
- (MZGLKitView *)getView {
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
		vc					= [[MZGLKitViewController alloc] initWithApp:app andGraphics:g];
		app->viewController = (__bridge void *) self;

		glView = (MZGLKitView *) vc.view;
#	else
		eventDispatcher = std::make_shared<EventDispatcher>(app);
		glView			= [[EventsView alloc] initWithFrame:self.view.frame eventDispatcher:eventDispatcher];
#	endif
		glView.frame = self.view.frame;
#	if !MZGL_IOS
		g->width  = self.view.frame.size.width * 2;
		g->height = self.view.frame.size.height * 2;
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
	//	audioUnit = [[MZGLEffectAU alloc] initWithPlugin: plugin andComponentDescription:desc error:error];
	//
	//	- (instancetype)initWithPlugin: (std::shared_ptr<Plugin>) _plugin
	//		   andComponentDescription:(AudioComponentDescription)componentDescription
	//						   options:(AudioComponentInstantiationOptions)options error:(NSError **)outError;
	audioUnit = [[MZGLEffectAU alloc] initWithPlugin:plugin andComponentDescription:desc error:error];
	NSLog(@"MZGL: createAudioUnitWithComponentDescription");

	//    if([NSThread isMainThread]) {
	//        NSLog(@"main thread");
	//        [self doBoth: audioUnit];
	//    } else {
	//        NSLog(@"dispatch");
	//        dispatch_async(dispatch_get_main_queue(), ^{
	//            NSLog(@"now on main thread");
	//            [self doBoth: audioUnit];
	//        });
	//    }

	return audioUnit;
}

@end
#endif
