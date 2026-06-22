//
//  MZGLES3Renderer.h
//  MZGLiOS
//
//  Created by Marek Bereza on 17/01/2018.
//  Copyright © 2018 Marek Bereza. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <MetalKit/MetalKit.h>
#include "App.h"

class EventDispatcher;
class Graphics;

// Metal render base. Owns the app / graphics / eventDispatcher and renders via
// sokol-metal in an MTKView (drawInMTKView). All interaction handling lives in
// EventsView (below). Mirror of the macOS MZMetalView.
@interface MZMetaliOSView : MTKView <MTKViewDelegate> {
@protected
	std::shared_ptr<App> app;
	std::shared_ptr<EventDispatcher> eventDispatcher;
	std::shared_ptr<Graphics> graphics;
}
- (std::shared_ptr<App>)getApp;
- (id)initWithApp:(std::shared_ptr<App>)_app andGraphics:(std::shared_ptr<Graphics>)_graphics;
- (std::shared_ptr<EventDispatcher>)getEventDispatcher;
- (BOOL)handleNormalOpen:(NSURL *)url;
- (void)deleteCppObjects;
@end

// Shared event-handling view. Inherits from the Metal render base and adds
// touch / keyboard / drag handling. This is the concrete view the controllers
// instantiate.
@interface EventsView : MZMetaliOSView
@end
