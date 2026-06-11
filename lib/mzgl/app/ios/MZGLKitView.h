//
//  MZGLES3Renderer.h
//  MZGLiOS
//
//  Created by Marek Bereza on 17/01/2018.
//  Copyright © 2018 Marek Bereza. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <GLKit/GLKit.h>
#ifdef MZGL_SOKOL
#	import <MetalKit/MetalKit.h>
#endif
#include "App.h"

class EventDispatcher;
class Graphics;

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

#ifdef MZGL_SOKOL
// Metal render base. Mirrors MZGLKitView's ownership/accessor surface so the
// shared EventsView can inherit from it, but renders via sokol-metal in an
// MTKView (drawInMTKView) instead of GLKView/drawRect. Mirror of the macOS
// MZMetalView.
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
#endif

// Shared event-handling view. Inherits from whichever render base is selected
// at compile time (GL or Metal) and adds touch / keyboard / drag handling, so
// both backends get identical interaction behaviour. This is the concrete view
// the controllers instantiate.
#ifdef MZGL_SOKOL
#	define MZGL_IOS_RENDER_BASE MZMetaliOSView
#else
#	define MZGL_IOS_RENDER_BASE MZGLKitView
#endif

@interface EventsView : MZGL_IOS_RENDER_BASE
@end
