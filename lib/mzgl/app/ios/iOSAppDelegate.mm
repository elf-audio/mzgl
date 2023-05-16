//
//  AppDelegate.m
//  MZGLiOS
//
//  Created by Marek Bereza on 16/01/2018.
//  Copyright © 2018 Marek Bereza. All rights reserved.
//

#import "iOSAppDelegate.h"
#import <GLKit/GLKit.h>
//#import "MZGLKitView.h"
#import "MZGLKitViewController.h"
#include "App.h"
#include "EventDispatcher.h"
#include "AudioShareSDK.h"
#include "log.h"
#include "PluginEditor.h"
#include "DateTime.h"

@interface iOSAppDelegate () {
	std::shared_ptr<App> app;
	Graphics g;
	MZGLKitViewController *mzViewController;
}
@end

@implementation iOSAppDelegate
@synthesize window;


- (BOOL)application:(UIApplication *)app openURL:(NSURL *)url options:(NSDictionary<UIApplicationOpenURLOptionsKey, id> *)options {
	//NSString* urlPath = url.path;
	NSLog(@"openURL: %@", url);
	
	auto eventDispatcher = [mzViewController getEventDispatcher];
	std::string urlStr = [[url absoluteString] UTF8String];
	bool isLocalFilePath = urlStr.size()>0 && urlStr[0]=='/';
	
	if([url.scheme containsString:@"audioshare"]) {
		
		if([[AudioShare sharedInstance] checkPendingImport:url withBlock:^(NSString *path) {
			
			
			// move file to docs dir
			// if it didn't fail, send to eventDispatcher, then delete it
			
			fs::path source([path UTF8String]);
			fs::path destination = docsPath() / source.filename();
			
			try {
				
				fs::rename(source, destination);
				eventDispatcher->openUrl(destination.string());
				fs::remove(destination);
				
			} catch(fs::filesystem_error &e) {
				Log::e() << "Couldn't copy file: " << e.what();
				return;
			}
			
		}]) {
			return YES;
		} else {
			return NO;
		}
	} else if([url.scheme containsString:@"audiobus"]) {
		// this is to prevent audiobus url schemas opening url's like a file.
		return YES;
	} else if([url.scheme containsString:@"file"] || isLocalFilePath) {
		return [[mzViewController getView] handleNormalOpen: url];
	} else {
		eventDispatcher->openUrl(urlStr);
	}
	return NO;
}


class ErrorApp : public App {
public:
	std::string msg;
	ErrorApp(Graphics &g, std::string msg) : App(g), msg(msg) {}
	void draw() override {
		g.clear(0);
		g.setColor(1);
		g.drawTextCentred(msg, vec2(g.width/2.f, g.height/2.f));
	}
};
- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions {
	
	
	// Override point for customization after application launch.
	self.window = [[UIWindow alloc] initWithFrame:[UIScreen mainScreen].bounds];
	
	
	
	try {
		if(isPlugin()) {
			app = instantiatePluginEditor(g, instantiatePlugin());
		} else {
			app = instantiateApp(g);

		}
		
		mzViewController = [[MZGLKitViewController alloc] initWithApp: app];
		window.rootViewController = mzViewController;
		app->viewController = (__bridge void*)mzViewController;
        app->windowHandle = (__bridge void*)window;
		
		
	} catch(std::exception &e) {
		writeStringToFile("instantiateAppError.txt", e.what());
		
		app = std::make_shared<ErrorApp>(g, e.what());
		
	}
	
	mzViewController = [[MZGLKitViewController alloc] initWithApp: app];
	window.rootViewController = mzViewController;
	app->viewController = (__bridge void*)mzViewController;

	
	[window makeKeyAndVisible];
	
	
	auto eventDispatcher = [mzViewController getEventDispatcher];
	NSURL *launchedUrl = [launchOptions objectForKey: UIApplicationLaunchOptionsURLKey];
	if(launchedUrl!=nil) {
		eventDispatcher->openUrl([[launchedUrl absoluteString] UTF8String]);
	}
	
	
	return YES;
}

- (void)applicationWillResignActive:(UIApplication *)application {
	// Sent when the application is about to move from active to inactive state. This can occur for certain types of temporary interruptions (such as an incoming phone call or SMS message) or when the user quits the application and it begins the transition to the background state.
	// Use this method to pause ongoing tasks, disable timers, and invalidate graphics rendering callbacks. Games should use this method to pause the game.
}


- (void)applicationDidEnterBackground:(UIApplication *)application {
	// Use this method to release shared resources, save user data, invalidate timers, and store enough application state information to restore your application to its current state in case it is terminated later.
	// If your application supports background execution, this method is called instead of applicationWillTerminate: when the user quits.
	auto eventDispatcher = [mzViewController getEventDispatcher];
	eventDispatcher->didEnterBackground();
}


- (void)applicationWillEnterForeground:(UIApplication *)application {
	// Called as part of the transition from the background to the active state; here you can undo many of the changes made on entering the background.
	auto eventDispatcher = [mzViewController getEventDispatcher];
	eventDispatcher->willEnterForeground();
}

- (void)applicationDidBecomeActive:(UIApplication *)application {
	// Restart any tasks that were paused (or not yet started) while the application was inactive. If the application was previously in the background, optionally refresh the user interface.
//	oslog("koaladebug applicationDidBecomeActive.");
}

- (void)applicationWillTerminate:(UIApplication *)application {
	// Called when the application is about to terminate. Save data if appropriate. See also applicationDidEnterBackground:.
	auto eventDispatcher = [mzViewController getEventDispatcher];
	eventDispatcher->exit();
}

-(void) dealloc {
	NSLog(@"dealloc iOSAppDelegate");
}

@end
