//
//  iOSAppDelegate.h
//  MZGLiOS
//
//  Created by Marek Bereza on 16/01/2018.
//  Copyright © 2018 Marek Bereza. All rights reserved.
//

#import <UIKit/UIKit.h>

// App-level delegate. Creates the mzgl App + root view controller at launch.
// The UIWindow itself is created by iOSSceneDelegate (UIScene lifecycle, required
// by the iOS 27 SDK) and handed back here via attachWindow: so legacy callers that
// read [[UIApplication sharedApplication].delegate window] keep working.
@interface iOSAppDelegate : UIResponder <UIApplicationDelegate>

@property(strong, nonatomic) UIWindow *window;

// The MZRootViewController created at launch (typed loosely to keep this header
// importable from plain Objective-C / C++ translation units).
- (UIViewController *)rootViewController;

// Called by the scene delegate once it has created the window for its scene.
- (void)attachWindow:(UIWindow *)window;

// Shared URL / user-activity handling, used by both the app-delegate callbacks
// (pre-scene hosts) and the scene delegate.
- (BOOL)handleOpenURL:(NSURL *)url;
- (BOOL)handleUserActivity:(NSUserActivity *)userActivity;

// Lifecycle forwarding from the scene delegate.
- (void)handleDidEnterBackground;
- (void)handleWillEnterForeground;

@end
