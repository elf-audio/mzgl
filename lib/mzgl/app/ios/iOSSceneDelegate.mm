//
//  iOSSceneDelegate.mm
//  MZGLiOS
//

#import "iOSSceneDelegate.h"
#import "iOSAppDelegate.h"

static iOSAppDelegate *appDelegate() {
	return (iOSAppDelegate *) [UIApplication sharedApplication].delegate;
}

@implementation iOSSceneDelegate

- (void)scene:(UIScene *)scene
	willConnectToSession:(UISceneSession *)session
				 options:(UISceneConnectionOptions *)connectionOptions {
	if (![scene isKindOfClass:[UIWindowScene class]]) return;
	UIWindowScene *windowScene = (UIWindowScene *) scene;

	self.window					   = [[UIWindow alloc] initWithWindowScene:windowScene];
	self.window.rootViewController = [appDelegate() rootViewController];
	[appDelegate() attachWindow:self.window];
	[self.window makeKeyAndVisible];

	// Cold launch via a URL (file open, custom scheme) or a universal link: the
	// app delegate's launchOptions / openURL: are not used under the scene
	// lifecycle, everything arrives in the connection options instead.
	for (UIOpenURLContext *ctx in connectionOptions.URLContexts) {
		[appDelegate() handleOpenURL:ctx.URL];
	}
	for (NSUserActivity *activity in connectionOptions.userActivities) {
		[appDelegate() handleUserActivity:activity];
	}
}

- (void)scene:(UIScene *)scene openURLContexts:(NSSet<UIOpenURLContext *> *)URLContexts {
	for (UIOpenURLContext *ctx in URLContexts) {
		[appDelegate() handleOpenURL:ctx.URL];
	}
}

- (void)scene:(UIScene *)scene continueUserActivity:(NSUserActivity *)userActivity {
	[appDelegate() handleUserActivity:userActivity];
}

- (void)sceneDidEnterBackground:(UIScene *)scene {
	[appDelegate() handleDidEnterBackground];
}

- (void)sceneWillEnterForeground:(UIScene *)scene {
	[appDelegate() handleWillEnterForeground];
}

@end
