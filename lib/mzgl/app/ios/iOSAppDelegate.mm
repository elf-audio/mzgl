//
//  AppDelegate.m
//  MZGLiOS
//
//  Created by Marek Bereza on 16/01/2018.
//  Copyright © 2018 Marek Bereza. All rights reserved.
//

#import "iOSAppDelegate.h"
#import "MZMetaliOSViewController.h"
#include "App.h"
#include "EventDispatcher.h"
#include "AudioShareSDK.h"
#include "log.h"
#include "PluginEditor.h"
#include "DateTime.h"
#include "ScopedUrl.h"
#include "util.h"
#include "mzAssert.h"

#if defined(__APPLE__)
void quitApplication() {
	mzAssert(false);
}
#endif

@interface iOSAppDelegate () {
	std::shared_ptr<App> app;
	std::shared_ptr<Plugin> plugin;
	std::shared_ptr<Graphics> g;
	MZRootViewController *mzViewController;
}
@end

@implementation iOSAppDelegate
@synthesize window;

- (UIViewController *)rootViewController {
	return mzViewController;
}

- (void)attachWindow:(UIWindow *)win {
	self.window		  = win;
	app->windowHandle = (__bridge void *) win;
}

// Pre-scene hosts only; under the UIScene lifecycle iOSSceneDelegate calls
// handleOpenURL: directly.
- (BOOL)application:(UIApplication *)application
			openURL:(NSURL *)url
			options:(NSDictionary<UIApplicationOpenURLOptionsKey, id> *)options {
	return [self handleOpenURL:url];
}

- (BOOL)handleOpenURL:(NSURL *)url {
	NSLog(@"openURL: %@", url);
	auto eventDispatcher = [mzViewController getEventDispatcher];
	std::string urlStr	 = [[url absoluteString] UTF8String];
	bool isLocalFilePath = urlStr.size() > 0 && urlStr[0] == '/';

	if ([url.scheme containsString:@"audioshare"]) {
		@try {
			if ([[AudioShare sharedInstance]
					checkPendingImport:url
							 withBlock:^(NSString *path) {
							   // move file to docs dir
							   // if it didn't fail, send to eventDispatcher, then delete it

							   fs::path source([path UTF8String]);
							   fs::path destination = docsPath() / source.filename();

							   try {
								   fs::rename(source, destination);
							   } catch (fs::filesystem_error &e) {
								   Log::e() << "Couldn't copy file: " << e.what();
								   return;
							   }
							   eventDispatcher->openUrl(ScopedUrl::createWithDeleter(destination));
							 }]) {
				return YES;
			} else {
				return NO;
			}
		} @catch (NSException *exception) {
			NSLog(@"AudioShare Exception occurred: %@, %@", exception.name, exception.reason);
			// You can also handle the exception as needed
			NSString *errorString =
				[NSString stringWithFormat:@"Error: %@, Reason: %@", exception.name, exception.reason];
			std::string errorCStr = [errorString UTF8String];
			eventDispatcher->app->dialogs.alert("AudioShare Error", errorCStr);
			return NO;
		}
	} else if ([url.scheme containsString:@"audiobus"]) {
		// this is to prevent audiobus url schemas opening url's like a file.
		return YES;
	} else if ([url.scheme containsString:@"file"] || isLocalFilePath) {
		return [[mzViewController getView] handleNormalOpen:url];
	} else {
		eventDispatcher->openUrl(ScopedUrl::create(urlStr));
	}
	return NO;
}

// Universal Links (e.g. https://koalaver.se/verify/<token>) arrive here both at
// cold launch and while running. The https URL is routed through the same
// openUrl() path as custom schemes.
- (BOOL)application:(UIApplication *)application
	continueUserActivity:(NSUserActivity *)userActivity
	  restorationHandler:(void (^)(NSArray<id<UIUserActivityRestoring>> *))restorationHandler {
	return [self handleUserActivity:userActivity];
}

- (BOOL)handleUserActivity:(NSUserActivity *)userActivity {
	if ([userActivity.activityType isEqualToString:NSUserActivityTypeBrowsingWeb] && userActivity.webpageURL) {
		auto eventDispatcher = [mzViewController getEventDispatcher];
		std::string url		 = [[userActivity.webpageURL absoluteString] UTF8String];
		eventDispatcher->openUrl(ScopedUrl::create(url));
		return YES;
	}
	return NO;
}

class ErrorApp : public App {
public:
	std::string msg;
	ErrorApp(Graphics &g, std::string msg)
		: App(g)
		, msg(msg) {}
	void draw() override {
		g.clear(0);
		g.setColor(1);

		float textWidth	 = g.width / 2;
		auto lines		 = g.getFont().wrapText(msg, textWidth);
		float lineHeight = g.getFont().getVerticalMetrics().lineHeight;
		float textHeight = lines.size() * lineHeight;

		float topLineY = g.height / 2 - textHeight / 2;
		g.drawTextHorizontallyCentred(
			"koala crashed with an unrecoverable error - pls take a screenshot and email to koala.helpdesk@gmail.com",
			{g.width / 2, topLineY - lineHeight * 2});

		for (int i = 0; i < lines.size(); i++) {
			g.drawText(lines[i], g.width / 2 - textWidth / 2, topLineY + i * lineHeight);
		}
	}
};

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions {
	// The UIWindow is created by iOSSceneDelegate in scene:willConnectToSession:
	// (UIScene lifecycle); this only builds the App and its root view controller.
	try {
		g = std::make_shared<Graphics>();
		if (isPlugin()) {
			plugin = instantiatePlugin();
			app	   = instantiatePluginEditor(*g, plugin);
		} else {
			app = instantiateApp(*g);
		}
	} catch (std::exception &e) {
		writeStringToFile("instantiateAppError.txt", e.what());
		app = std::make_shared<ErrorApp>(*g, e.what());
	}

	mzViewController	= [[MZRootViewController alloc] initWithApp:app andGraphics:g];
	app->viewController = (__bridge void *) mzViewController;

	return YES;
}

- (UISceneConfiguration *)application:(UIApplication *)application
	configurationForConnectingSceneSession:(UISceneSession *)connectingSceneSession
								   options:(UISceneConnectionOptions *)options {
	UISceneConfiguration *config = [[UISceneConfiguration alloc] initWithName:@"Default"
																  sessionRole:connectingSceneSession.role];
	config.delegateClass		 = NSClassFromString(@"iOSSceneDelegate");
	return config;
}

- (void)applicationWillResignActive:(UIApplication *)application {
	// Sent when the application is about to move from active to inactive state. This can occur for certain types of temporary interruptions (such as an incoming phone call or SMS message) or when the user quits the application and it begins the transition to the background state.
	// Use this method to pause ongoing tasks, disable timers, and invalidate graphics rendering callbacks. Games should use this method to pause the game.
}

// Under the UIScene lifecycle these two app-delegate callbacks are no longer sent;
// iOSSceneDelegate forwards sceneDidEnterBackground / sceneWillEnterForeground to
// the handle* methods instead.
- (void)applicationDidEnterBackground:(UIApplication *)application {
	[self handleDidEnterBackground];
}

- (void)applicationWillEnterForeground:(UIApplication *)application {
	[self handleWillEnterForeground];
}

- (void)handleDidEnterBackground {
	auto eventDispatcher = [mzViewController getEventDispatcher];
	eventDispatcher->didEnterBackground();
}

- (void)handleWillEnterForeground {
	auto eventDispatcher = [mzViewController getEventDispatcher];
	eventDispatcher->willEnterForeground();
}

- (void)applicationDidBecomeActive:(UIApplication *)application {
	// Restart any tasks that were paused (or not yet started) while the application was inactive. If the application was previously in the background, optionally refresh the user interface.
	//	oslog("koaladebug applicationDidBecomeActive.");
}

- (void)applicationWillTerminate:(UIApplication *)application {
	// Called when the application is about to terminate. Save data if appropriate. See also applicationDidEnterBackground:.
	{
		auto eventDispatcher = [mzViewController getEventDispatcher];
		eventDispatcher->exit();
	}

	// do not rely on iOS to destroy your shared pointers in iOSAppDelegate - for
	// some reason, iOSAppDelegate is not destroyed conventionally, so deleting
	// all the mzgl shared_ptrs owned by it and the objects it owns is the only
	// way I can work out to make sure ~App() is called.
	[mzViewController deleteCppObjects];
	app = nullptr;

	// destroy plugin last! it must live longer than app.
	plugin = nullptr;
	g = nullptr;
}

@end
