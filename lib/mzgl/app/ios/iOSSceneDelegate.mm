//
//  iOSSceneDelegate.mm
//  MZGLiOS
//

#import "iOSSceneDelegate.h"
#import "iOSAppDelegate.h"

static iOSAppDelegate *appDelegate() {
	return (iOSAppDelegate *) [UIApplication sharedApplication].delegate;
}

@implementation iOSSceneDelegate {
	// True only for the app's own window scene. iOS remembers which delegate a scene
	// session was given, so an external-display session created by an older build
	// (which gave every role this delegate) can still arrive here. Such a scene must
	// not take the root view controller, and its lifecycle must not reach the app:
	// a monitor being unplugged is not the app going to the background.
	BOOL isAppScene;
}

- (void)scene:(UIScene *)scene
	willConnectToSession:(UISceneSession *)session
				 options:(UISceneConnectionOptions *)connectionOptions {
	if (![scene isKindOfClass:[UIWindowScene class]]) return;
	if (![session.role isEqualToString:UIWindowSceneSessionRoleApplication]) return;
	isAppScene = YES;
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
	if (!isAppScene) return;
	for (UIOpenURLContext *ctx in URLContexts) {
		[appDelegate() handleOpenURL:ctx.URL];
	}
}

- (void)scene:(UIScene *)scene continueUserActivity:(NSUserActivity *)userActivity {
	if (!isAppScene) return;
	[appDelegate() handleUserActivity:userActivity];
}

- (void)sceneDidEnterBackground:(UIScene *)scene {
	if (!isAppScene) return;
	[appDelegate() handleDidEnterBackground];
}

- (void)sceneWillEnterForeground:(UIScene *)scene {
	if (!isAppScene) return;
	[appDelegate() handleWillEnterForeground];
}

@end
