//
//  EventsView.h
//  mzgl macOS
//
//  Created by Marek Bereza on 03/02/2021.
//  Copyright © 2021 Marek Bereza. All rights reserved.
//

#pragma once

#import <Cocoa/Cocoa.h>

// Render-view base class per graphics backend: the Sokol and native-Metal
// backends both draw through an MTKView (MZMetalView); OpenGL uses the
// NSOpenGLView-based MZGLView.
#if defined(MZGL_SOKOL) || defined(MZGL_METAL)
#	include "MZMetalView.h"
#	define GL_VIEW_CLASS MZMetalView
#else
#	include "MZGLView.h"
#	define GL_VIEW_CLASS MZGLView
#endif

@interface EventsView : GL_VIEW_CLASS <NSWindowDelegate>
- (id)initWithFrame:(NSRect)frame eventDispatcher:(std::shared_ptr<EventDispatcher>)evtDispatcher;
- (void)shutdown;

// When YES (default), the view becomes first responder and consumes key events.
// Set to NO when embedded in a plugin host (e.g. Ableton Live) where the host
// owns keyboard shortcuts and the plugin shouldn't steal them.
@property (nonatomic, assign) BOOL handlesKeyboard;
@end
