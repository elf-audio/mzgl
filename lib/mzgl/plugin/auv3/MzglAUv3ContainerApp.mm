// Generic container application for an mzgl AUv3 (added by mzgl_add_auv3_plugin).
//
// macOS only discovers Audio Unit v3 extensions that live inside an application
// bundle (<app>/Contents/PlugIns/<name>.appex), so an AUv3 always ships with a host
// app. This one has no audio of its own: it tells the user the Audio Unit is
// installed, registers it with pluginkit, and quits.
//
// Compiled with MZGL_AUV3_PRODUCT_NAME / MZGL_AUV3_AU_NAME string defines.
#import <Cocoa/Cocoa.h>

#ifndef MZGL_AUV3_PRODUCT_NAME
#	define MZGL_AUV3_PRODUCT_NAME "mzgl plugin"
#endif
#ifndef MZGL_AUV3_AU_NAME
#	define MZGL_AUV3_AU_NAME MZGL_AUV3_PRODUCT_NAME
#endif

@interface MzglAUv3ContainerAppDelegate : NSObject <NSApplicationDelegate>
@property(strong) NSWindow *window;
@end

@implementation MzglAUv3ContainerAppDelegate

- (void)registerExtension {
	// Same thing an installer's postinstall does: register the embedded appex so
	// hosts (Logic, GarageBand, auval) see it without a re-login.
	NSString *plugIns = [[NSBundle mainBundle] builtInPlugInsPath];
	NSArray *items	  = [[NSFileManager defaultManager] contentsOfDirectoryAtPath:plugIns error:nil];
	for (NSString *item in items) {
		if (![item hasSuffix:@".appex"]) continue;
		NSTask *task		= [[NSTask alloc] init];
		task.launchPath		= @"/usr/bin/pluginkit";
		task.arguments		= @[ @"-a", [plugIns stringByAppendingPathComponent:item] ];
		task.standardOutput = [NSFileHandle fileHandleWithNullDevice];
		task.standardError	= [NSFileHandle fileHandleWithNullDevice];
		@try {
			[task launch];
		} @catch (NSException *e) {
			NSLog(@"%s: pluginkit failed: %@", MZGL_AUV3_PRODUCT_NAME, e);
		}
	}
}

- (void)buildMenu {
	NSMenu *mainMenu	= [[NSMenu alloc] init];
	NSMenuItem *appItem = [[NSMenuItem alloc] init];
	NSMenu *appMenu		= [[NSMenu alloc] init];
	NSString *appName	= @MZGL_AUV3_PRODUCT_NAME;
	[appMenu addItemWithTitle:[@"About " stringByAppendingString:appName]
					   action:@selector(orderFrontStandardAboutPanel:)
				keyEquivalent:@""];
	[appMenu addItem:[NSMenuItem separatorItem]];
	[appMenu addItemWithTitle:[@"Quit " stringByAppendingString:appName]
					   action:@selector(terminate:)
				keyEquivalent:@"q"];
	appItem.submenu = appMenu;
	[mainMenu addItem:appItem];
	[NSApp setMainMenu:mainMenu];
}

- (void)applicationDidFinishLaunching:(NSNotification *)notification {
	[self buildMenu];
	[self registerExtension];

	NSRect frame = NSMakeRect(0, 0, 460, 260);
	self.window	 = [[NSWindow alloc]
		initWithContentRect:frame
				  styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable
					backing:NSBackingStoreBuffered
					  defer:NO];
	self.window.title = @MZGL_AUV3_PRODUCT_NAME;
	[self.window center];

	NSView *content = self.window.contentView;

	NSImageView *icon = [NSImageView imageViewWithImage:[NSApp applicationIconImage]];
	icon.frame		  = NSMakeRect(24, 140, 96, 96);
	[content addSubview:icon];

	NSTextField *title = [NSTextField labelWithString:@MZGL_AUV3_PRODUCT_NAME];
	title.font		   = [NSFont boldSystemFontOfSize:20];
	title.frame		   = NSMakeRect(140, 200, 300, 30);
	[content addSubview:title];

	NSString *bodyText = [NSString
		stringWithFormat:@"%s is installed as an Audio Unit (AUv3).\n\n"
						 @"Open it inside Logic Pro, GarageBand or any other Audio Unit host - "
						 @"look for \"%s\" in the plug-in list.\n\n"
						 @"This app only needs to exist so macOS can find the plug-in.",
						 MZGL_AUV3_PRODUCT_NAME, MZGL_AUV3_AU_NAME];
	NSTextField *body = [NSTextField wrappingLabelWithString:bodyText];
	body.font		  = [NSFont systemFontOfSize:13];
	body.frame		  = NSMakeRect(140, 50, 296, 150);
	[content addSubview:body];

	NSButton *quit	   = [NSButton buttonWithTitle:@"Quit" target:NSApp action:@selector(terminate:)];
	quit.keyEquivalent = @"\r";
	quit.frame		   = NSMakeRect(356, 14, 80, 32);
	[content addSubview:quit];

	[self.window makeKeyAndOrderFront:nil];
	[NSApp activateIgnoringOtherApps:YES];
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender {
	return YES;
}

@end

int main(int argc, const char *argv[]) {
	@autoreleasepool {
		NSApplication *app = [NSApplication sharedApplication];
		[app setActivationPolicy:NSApplicationActivationPolicyRegular];
		MzglAUv3ContainerAppDelegate *delegate = [[MzglAUv3ContainerAppDelegate alloc] init];
		app.delegate						   = delegate;
		[app run];
	}
	return 0;
}
