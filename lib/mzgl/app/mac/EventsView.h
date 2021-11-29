//
//  EventsView.h
//  mzgl macOS
//
//  Created by Marek Bereza on 03/02/2021.
//  Copyright © 2021 Marek Bereza. All rights reserved.
//

#pragma once

#import <Cocoa/Cocoa.h>
#ifdef USE_METALANGLE
#include "MZMGLKView.h"
#define GL_VIEW_CLASS MZMGLKView

#else
#include "MZGLView.h"
#define GL_VIEW_CLASS MZGLView
#endif


@interface EventsView : GL_VIEW_CLASS<NSWindowDelegate>
- (id) initWithFrame: (NSRect) frame eventDispatcher:(void*)evtDispatcherPtr;
- (void) shutdown;
- (void*) getApp;

@end

