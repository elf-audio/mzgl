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

class AppHolder {
public:
    Graphics g;
    PluginEditor *app;
    shared_ptr<EventDispatcher> eventDispatcher;
};


@implementation AudioUnitViewController {
    //__weak - WARNING DOES THIS CRASH iOS?
//    __unsafe_unretained MZGLEffectAU *audioUnit;
//#if TARGET_OS_IPHONE
    MZGLKitViewController *vc;
    MZGLKitView *glView;
    Graphics g;
    PluginEditor *app;
    Plugin *plugin;
//#else
//    MZGLView *glView;
//    AppHolder appHolder;
//#endif
    BOOL needsConnection;
}
#include <thread>

- (id) init {
    self = [super init];
    if(self!=nil) {
        needsConnection = YES;
        
#if !TARGET_OS_IOS
        auto &g = appHolder.g;
        appHolder.app = std::shared_ptr<App>(instantiateApp(g));
        self.view = [[NSView alloc] initWithFrame: CGRectMake(0, 0, g.width/(float)g.pixelScale, g.height/(float)g.pixelScale)];
        [self setPreferredContentSize: CGSizeMake(g.width/(float)g.pixelScale, g.height/(float)g.pixelScale)];
        appHolder.eventDispatcher = std::make_shared<EventDispatcher>(appHolder.app);
        glView = [[MZGLView alloc] initWithFrame: self.view.frame eventDispatcher: appHolder.eventDispatcher.get()];


//        [self addGLView];
//        [self connectViewToAU];
#else
        [self setPreferredContentSize: CGSizeMake(500, 800)];
#endif
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

- (void) viewDidLoad {
    NSLog(@"MZGL: viewDidLoad");
    [super viewDidLoad];

//    [self doBoth];

    //[glView setNeedsDisplay];
}

- (void) doBoth: (MZGLEffectAU*) audioUnit {
    if(app==nullptr && audioUnit!=nil) {
        app = instantiatePluginEditor(g, (Plugin*)[audioUnit getPlugin]);
        vc = [[MZGLKitViewController alloc] initWithApp: app];
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
    
    NSLog(@"MZGL: createAudioUnitWithComponentDescription");
    MZGLEffectAU *audioUnit = [[MZGLEffectAU alloc] initWithComponentDescription:desc error:error];
    plugin = (Plugin*)[audioUnit getPlugin];
    if([NSThread isMainThread]) {
        NSLog(@"main thread");
        [self doBoth: audioUnit];
    } else {
        NSLog(@"dispatch");
        dispatch_async(dispatch_get_main_queue(), ^{
            NSLog(@"now on main thread");
            [self doBoth: audioUnit];
        });
        
    }
    NSLog(@"ready to return");
    
    return audioUnit;
}

- (void) dealloc {
    // don't delete the app, the view will delete it
    // when it falls out of scope (shared ptr)
//    if(app!=nullptr) delete app;
    glView = nil;
    delete app;
    delete plugin;
    printf("Finished\n");
}
@end
