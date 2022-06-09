//
//  MZOpenGLView.h - this is the OpenGL View wrapper
//  MZGL
//
//  Created by Marek Bereza on 15/01/2018.
//  Copyright © 2018 Marek Bereza. All rights reserved.
//

#pragma once

#import <Cocoa/Cocoa.h>

//#include "App.h"
class EffectPlugin;
#ifdef USE_METALANGLE
#import <MetalANGLE/MGLKView.h>
#else

#endif
class EventDispatcher;
@interface MZGLView : NSOpenGLView {
	EventDispatcher *eventDispatcher;
}

- (id) initWithFrame: (NSRect) frame eventDispatcher:(void*)evtDispatcherPtr;
- (void)windowResized:(NSNotification *)notification;
- (void) disableDrawing;
- (void) enableDrawing;
- (void) lock;
- (void) unlock;
- (void) shutdown;
@property (nonatomic, readwrite, retain) IBOutlet MZGLView *view;

@end
