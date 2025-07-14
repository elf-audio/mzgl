//
//  EventsView.h
//  mzgl macOS
//
//  Created by Marek Bereza on 03/02/2021.
//  Copyright © 2021 Marek Bereza. All rights reserved.
//

#pragma once

#import <Cocoa/Cocoa.h>

#ifdef MZGL_SOKOL_METAL
#	include "MZMetalView.h"
#	define GL_VIEW_CLASS MZMetalView
#else
#	include "MZGLView.h"
#	define GL_VIEW_CLASS MZGLView
#endif

@interface EventsView : GL_VIEW_CLASS <NSWindowDelegate>
- (id)initWithFrame:(NSRect)frame eventDispatcher:(std::shared_ptr<EventDispatcher>)evtDispatcher;
- (void)shutdown;
@end
