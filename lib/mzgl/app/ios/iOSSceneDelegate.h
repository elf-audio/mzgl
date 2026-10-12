//
//  iOSSceneDelegate.h
//  MZGLiOS
//
//  UIScene lifecycle delegate. The iOS 27 SDK traps at launch
//  (__UIApplicationEvaluateRuntimeIssueForNoSceneLifecycleAdoption) if an app
//  still relies on the pre-iOS 13 UIApplicationDelegate-only lifecycle, so the
//  window is created here and lifecycle/URL events are forwarded to
//  iOSAppDelegate, which still owns the App and root view controller.
//

#import <UIKit/UIKit.h>

@interface iOSSceneDelegate : UIResponder <UIWindowSceneDelegate>

@property(strong, nonatomic) UIWindow *window;

@end
