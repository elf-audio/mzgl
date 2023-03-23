//
//  MacAppDelegate.h
//  MZGL
//
//  Created by Marek Bereza on 15/01/2018.
//  Copyright © 2018 Marek Bereza. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#include <memory>


class EventDispatcher;
class App;

@interface MacAppDelegate : NSObject <NSApplicationDelegate> {}

-(id) initWithApp: (std::shared_ptr<App>)_app;

#ifdef UNIT_TEST
- (std::shared_ptr<App>) getApp;
- (EventDispatcher*) getEventDispatcher;
#endif
@end

