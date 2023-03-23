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
#include "Graphics.h"
#include "App.h"

Graphics g;
int main(int argc, const char * argv[]) {
	[NSApplication sharedApplication];
	[NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];

	loadCommandLineArgs(argc, argv);

	auto app = instantiateApp(g);
	if(!app->isHeadless()) {
		[NSApp activateIgnoringOtherApps:YES];
		id appDelegate = [[MacAppDelegate alloc] initWithApp: app];
		[NSApp setDelegate:appDelegate];
		[NSApp run];
	}
	return 0;
}


