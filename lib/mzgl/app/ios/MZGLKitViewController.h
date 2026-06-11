//
//  MZGLES3Renderer.h
//  MZGLiOS
//
//  Created by Marek Bereza on 17/01/2018.
//  Copyright © 2018 Marek Bereza. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <GLKit/GLKit.h>
#ifdef MZGL_PLUGIN
#	import <CoreAudioKit/CoreAudioKit.h>
#endif
#import "MZGLKitView.h"
class App;
class Graphics;
class EventDispatcher;
@interface MZGLKitViewController
	: GLKViewController
#ifdef MZGL_PLUGIN
	  // NOT SURE IF WE NEED THESE, AS THERES A DIFFERENT CLASS DOING IT I THINK
	  <AUAudioUnitFactory, NSExtensionRequestHandling, GLKViewControllerDelegate>
#else
	  <GLKViewControllerDelegate>
#endif

- (id)initWithApp:(std::shared_ptr<App>)app andGraphics:(std::shared_ptr<Graphics>)_graphics;
- (std::shared_ptr<EventDispatcher>)getEventDispatcher;
//- (void) openURLWhenLoadedAndDeleteFile: (NSString*) urlToOpen;
- (EventsView *)getView;
- (void) deleteCppObjects;
@end

#ifdef MZGL_SOKOL
// Metal counterpart of MZGLKitViewController. Plain UIViewController (an MTKView
// self-drives, so it doesn't need GLKViewController). Replicates the same
// resize / touch-delay / ownership behaviour.
@interface MZMetaliOSViewController : UIViewController
- (id)initWithApp:(std::shared_ptr<App>)app andGraphics:(std::shared_ptr<Graphics>)_graphics;
- (std::shared_ptr<EventDispatcher>)getEventDispatcher;
- (EventsView *)getView;
- (void)deleteCppObjects;
@end
typedef MZMetaliOSViewController MZRootViewController;
#else
typedef MZGLKitViewController MZRootViewController;
#endif
