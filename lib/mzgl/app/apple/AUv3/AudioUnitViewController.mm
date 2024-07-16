//
//  AudioUnitViewController.m
//  MZGLiOSEffectAU
//
//  Created by Marek Bereza on 05/04/2019.
//  Copyright © 2019 Marek Bereza. All rights reserved.
//

#import "AudioUnitViewController.h"
#import "MZGLEffectAU.h"
#if TARGET_OS_IPHONE
#	import <mzgl/app/ios/MZGLKitViewController.h>
#	import <mzgl/app/ios/MZGLKitView.h>
#else
#	import <mzgl/app/mac/EventsView.h>
#endif
#include <mzgl/Plugin.h>
#include <mzgl/PluginEditor.h>
#include <mzgl/util/EventDispatcher.h>

@interface AudioUnitViewController ()

@end

using namespace std;

@implementation AudioUnitViewController {
#if TARGET_OS_IPHONE
	MZGLKitViewController *vc;
	MZGLKitView *glView;
#else
	MZGLView *glView;
	std::shared_ptr<EventDispatcher> eventDispatcher;
#endif
	Graphics g;
	MZGLEffectAU *audioUnit;

	std::shared_ptr<Plugin> plugin;
	std::shared_ptr<PluginEditor> app;
}

#include <thread>

- (id)init {
	self = [super init];
	if (self != nil) {
		[self setup];
	}
	return self;
}

- (void)setup {
	app		  = nullptr;
	plugin	  = nullptr;
	audioUnit = nil;
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
	[self setPreferredContentSize:CGSizeMake(1100, 650)];
	//#endif
}
#if !TARGET_OS_IOS

- (instancetype)initWithNibName:(NSNibName)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil {
	self = [super initWithNibName:nibNameOrNil bundle:[NSBundle bundleForClass:self.class]];
	if (self != nil) {
		[self setup];
	}
	return self;
}
#endif

#if TARGET_OS_IOS
- (MZGLKitView *)getView {
	return glView;
}
#else
- (MZGLView *)getView {
	return glView;
}
#endif

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
#if TARGET_OS_IOS
		auto ed = [glView getEventDispatcher];
		if (ed != nullptr && ed->hasSetup()) {
			[vc viewWillTransitionToSize:self.view.window.frame.size withTransitionCoordinator:nil];
		}
#endif

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

- (void)dealloc {
    [self->glView removeObserver:self forKeyPath:@"frame"];
    [super dealloc];
}

- (void)observeValueForKeyPath:(NSString *)keyPath
                      ofObject:(id)object
                        change:(NSDictionary<NSKeyValueChangeKey,id> *)change
                       context:(void *)context {
    if ([keyPath isEqualToString:@"frame"] && object == self->glView) {
        g.width  = self.view.frame.size.width * 2;
        g.height = self.view.frame.size.height * 2;
//        eventDispatcher->resized();
    } else {
        [super observeValueForKeyPath:keyPath ofObject:object change:change context:context];
    }
}

- (void)viewWillAppear:(BOOL)animated {
	if (app == nullptr) {
		plugin = [self getPlugin];
		app	   = instantiatePluginEditor(g, plugin);
#if TARGET_OS_IOS
		vc					= [[MZGLKitViewController alloc] initWithApp:app];
		app->viewController = (__bridge void *) self;

		glView = (MZGLKitView *) vc.view;
#else
		eventDispatcher = std::make_shared<EventDispatcher>(app);
		glView			= [[EventsView alloc] initWithFrame:self.view.frame eventDispatcher:eventDispatcher];
        glView.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
        [glView setTranslatesAutoresizingMaskIntoConstraints:NO];

        NSLayoutConstraint *width = [NSLayoutConstraint
            constraintWithItem:glView
            attribute:NSLayoutAttributeWidth
            relatedBy:NSLayoutRelationEqual
            toItem:glView.superview
            attribute:NSLayoutAttributeWidth
            multiplier:1.0
            constant:0];
        NSLayoutConstraint *height = [NSLayoutConstraint
            constraintWithItem:glView
            attribute:NSLayoutAttributeHeight
            relatedBy:NSLayoutRelationEqual
            toItem:glView.superview
            attribute:NSLayoutAttributeHeight
            multiplier:1.0
            constant:0];

        [glView.superview addConstraints:@[width, height]];
#endif
		glView.frame = self.view.frame;
#if !TARGET_OS_IOS
        g.width	 = self.view.frame.size.width * 2;
        g.height = self.view.frame.size.height * 2;
		eventDispatcher->resized();
        
        [glView addObserver:self
                      forKeyPath:@"frame"
                         options:(NSKeyValueObservingOptionOld | NSKeyValueObservingOptionNew)
                         context:NULL];
#endif
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
    //self->glView.frame = CGRectMake(0, 0, size.width, size.height);
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
	NSLog(@"MZGL: createAudioUnitWithComponentDescription floopy ");

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
