//
//  MZGLKitViewController.h
//  MZGLiOS
//
//  Created by Marek Bereza on 17/01/2018.
//  Copyright © 2018 Marek Bereza. All rights reserved.
//
//  GL root view controller (OpenGL backend only). A GLKViewController that
//  owns the shared EAGLContext and drives the GLKView-based EventsView.

#import <Foundation/Foundation.h>
#import <GLKit/GLKit.h>
#import "MZGLKitView.h"
class App;
class Graphics;
class EventDispatcher;

// Note: the AUv3 factory/extension protocols live on AudioUnitViewController
// (app/apple/AUv3), not here - this controller only drives the GL view.
@interface MZGLKitViewController : GLKViewController <GLKViewControllerDelegate>

- (id)initWithApp:(std::shared_ptr<App>)app andGraphics:(std::shared_ptr<Graphics>)_graphics;
- (std::shared_ptr<EventDispatcher>)getEventDispatcher;
- (EventsView *)getView;
- (void)deleteCppObjects;
@end
