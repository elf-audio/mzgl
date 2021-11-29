//
//  MacAppDelegate.h
//  MZGL
//
//  Created by Marek Bereza on 15/01/2018.
//  Copyright © 2018 Marek Bereza. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#ifdef UNIT_TEST
class App;
class EventDispatcher;
#endif
@interface MacAppDelegate : NSObject <NSApplicationDelegate> {}

-(id) initWithApp: (void*)_app;

#ifdef UNIT_TEST
- (App*) getApp;
- (EventDispatcher*) getEventDispatcher;
#endif
@end

