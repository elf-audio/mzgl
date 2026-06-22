//
//  MZMetaliOSViewController.h
//  MZGLiOS
//
//  Created by Marek Bereza on 17/01/2018.
//  Copyright © 2018 Marek Bereza. All rights reserved.
//

#import <Foundation/Foundation.h>
#ifdef MZGL_PLUGIN
#	import <CoreAudioKit/CoreAudioKit.h>
#endif
#import "MZGLKitView.h"
class App;
class Graphics;
class EventDispatcher;

// Metal root view controller. A plain UIViewController (the MTKView self-drives,
// so it doesn't need GLKViewController). Replicates the resize / touch-delay /
// ownership behaviour the app relies on.
@interface MZMetaliOSViewController : UIViewController
- (id)initWithApp:(std::shared_ptr<App>)app andGraphics:(std::shared_ptr<Graphics>)_graphics;
- (std::shared_ptr<EventDispatcher>)getEventDispatcher;
- (EventsView *)getView;
- (void) deleteCppObjects;
@end

typedef MZMetaliOSViewController MZRootViewController;
