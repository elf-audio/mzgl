//
//  MZGLES3Renderer.h
//  MZGLiOS
//
//  Created by Marek Bereza on 17/01/2018.
//  Copyright © 2018 Marek Bereza. All rights reserved.
//

#import <Foundation/Foundation.h>
#include "backendDefines.h"
#ifdef MZGL_OPENGL
#	import <GLKit/GLKit.h>
#else
#	import <MetalKit/MetalKit.h>
#endif
#include "App.h"

class EventDispatcher;
class Graphics;

#ifdef MZGL_OPENGL

// GL render base. Owns the app / graphics / eventDispatcher and performs the
// OpenGL ES rendering (drawRect). All interaction handling lives in EventsView
// (below) so it can be shared verbatim with the Metal render base.
@interface MZGLKitView : GLKView {
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

#	define MZGL_IOS_RENDER_BASE MZGLKitView

#else // MZGL_SOKOL / MZGL_METAL

// Metal render base. Owns the app / graphics / eventDispatcher and renders in
// an MTKView (drawInMTKView) - via sokol-metal for the Sokol backend, or via
// MetalAPI begin/endFrame for the native Metal backend. All interaction
// handling lives in EventsView (below). Mirror of the macOS MZMetalView.
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

#	define MZGL_IOS_RENDER_BASE MZMetaliOSView

#endif

// Shared event-handling view. Inherits from whichever render base is selected
// at compile time and adds touch / keyboard / drag handling, so all backends
// get identical interaction behaviour. This is the concrete view the
// controllers instantiate.
@interface EventsView : MZGL_IOS_RENDER_BASE
@end
