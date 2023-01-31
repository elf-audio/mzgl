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
#import "MZGLKitViewController.h"
#import "MZGLKitView.h"
#else
#import "MZGLView.h"
#endif
#include "Plugin.h"
#include "PluginEditor.h"
#include "EventDispatcher.h"

@interface AudioUnitViewController ()

@end

using namespace std;
//
//class AppHolder {
//public:
//    Graphics g;
//    PluginEditor *app;
//    shared_ptr<EventDispatcher> eventDispatcher;
//};


@implementation AudioUnitViewController {
    //__weak - WARNING DOES THIS CRASH iOS?
//    __unsafe_unretained MZGLEffectAU *audioUnit;
//#if TARGET_OS_IPHONE
    MZGLKitViewController *vc;
    MZGLKitView *glView;
    Graphics g;
	MZGLEffectAU *audioUnit;
	
	
	std::shared_ptr<Plugin> plugin;
	std::shared_ptr<PluginEditor> app;
//#else
//    MZGLView *glView;
//    AppHolder appHolder;
//#endif

}
#include <thread>

- (id) init {
    self = [super init];
    if(self!=nil) {
	
		app = nullptr;
		plugin = nullptr;
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
        [self setPreferredContentSize: CGSizeMake(500, 800)];
//#endif
    }
    return self;
}

-(MZGLKitView*) getView {
    return glView;
}

- (void)didReceiveMemoryWarning {
    EventDispatcher *ed = [glView getEventDispatcher];
    if(ed != nullptr && ed->hasSetup()) {
        ed->memoryWarning();
    }
}

-(void) viewDidDisappear {
    [[self.view subviews] makeObjectsPerformSelector:@selector(removeFromSuperview)];
}

-(void) addGLView {
    // strip out all subviews first
    [[self.view subviews] makeObjectsPerformSelector:@selector(removeFromSuperview)];
    [self.view addSubview:glView];
}

- (void)viewDidLayoutSubviews {
    [self tryToResize];
}

-(void) tryToResize {
    if(self.view.window!=nil && glView!=nil) {
        glView.frame = self.view.frame;
    #if TARGET_OS_IPHONE
        EventDispatcher *ed = [glView getEventDispatcher];
        if(ed != nullptr && ed->hasSetup()) {
            [vc viewWillTransitionToSize: self.view.window.frame.size withTransitionCoordinator:nil];
        }
    #endif
        
    } else {
        NSLog(@"Window null");
    }
}

- (void)viewWillAppear:(BOOL)animated {
	if(app==nullptr && plugin!=nullptr) {
		app = std::shared_ptr<PluginEditor>(instantiatePluginEditor(g, plugin.get()));
		vc = [[MZGLKitViewController alloc] initWithApp: app.get()];
		app->viewController = (__bridge void*)self;

		glView = (MZGLKitView*)vc.view;
		glView.frame = self.view.frame;
		[self addGLView];
	}
}


-(void) viewDidAppear:(BOOL)animated {
    NSLog(@"MZGL: viewDidAppear");
    [self addGLView];
    [self tryToResize];
}

- (void)viewWillTransitionToSize:(CGSize)size withTransitionCoordinator:(id)coordinator {
    NSLog(@"MZGL: Size change whaaa");
}

 
- (AUAudioUnit *)createAudioUnitWithComponentDescription:(AudioComponentDescription)desc error:(NSError **)error {
    
	audioUnit = [[MZGLEffectAU alloc] initWithComponentDescription:desc error:error];
	plugin = [audioUnit getPlugin];
	
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
