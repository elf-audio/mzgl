//
//  main.m
//  MZGLiOS
//
//  Created by Marek Bereza on 16/01/2018.
//  Copyright © 2018 Marek Bereza. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "iOSAppDelegate.h"

int main(int argc, char * argv[]) {
	NSString * appDelegateClassName;

	@autoreleasepool {
	    // Setup code that might create autoreleased objects goes here.
	    appDelegateClassName = NSStringFromClass([iOSAppDelegate class]);
	}
	
	return UIApplicationMain(argc, argv, nil, appDelegateClassName);
}
