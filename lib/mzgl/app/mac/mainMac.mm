//
//  main.m
//  MZGL
//
//  Created by Marek Bereza on 15/01/2018.
//  Copyright © 2018 Marek Bereza. All rights reserved.
//

#import <Cocoa/Cocoa.h>

#include "MacAppDelegate.h"
#include "util.h"
#include <mzgl/gl/Graphics.h>
#include <mzgl/App.h>
#include <mzgl/util/mzAssert.h>

#if defined (__APPLE__)
int main(int argc, const char *argv[]) __attribute__((used));
#endif

Graphics g;
int main(int argc, const char *argv[]) {
	[NSApplication sharedApplication];
	[NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];

	loadCommandLineArgs(argc, argv);

	auto app = instantiateApp(g);
	if (app->isHeadless()) {
		return 0;
	}

	[NSApp activateIgnoringOtherApps:YES];
	id appDelegate = [[MacAppDelegate alloc] initWithApp:app];
	[NSApp setDelegate:appDelegate];

	//
	// app is quit with [NSApp terminate:] - according to docs
	//
	// 	"Don’t bother to put final cleanup code in your app’s
	// 	 main() function—it will never be executed. If cleanup is
	//   necessary, perform that cleanup in the delegate’s
	// 	 applicationWillTerminate: method."
	//
	// Delightful...
	//
	// so that means lets not retain a copy of the app here
	// or the app's use count will never hit zero and not be deleted.
	app = nullptr;

	[NSApp run];

	return 0;
}
