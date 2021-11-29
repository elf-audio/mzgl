//
//  MZMGLKView.h
//  mzgl macOS - this is the MetalANGLE wrapper
//
//  Created by Marek Bereza on 03/02/2021.
//  Copyright © 2021 Marek Bereza. All rights reserved.
//



#pragma once
#import <Cocoa/Cocoa.h>
#import <MetalANGLE/MGLKView.h>

class EventDispatcher;

@interface MZMGLKView : MGLKView {
	EventDispatcher *eventDispatcher;
}

- (id) initWithFrame: (NSRect) frame eventDispatcher:(void*)evtDispatcherPtr;
- (void)windowResized:(NSNotification *)notification;
- (void) lock;
- (void) unlock;
- (void) shutdown;
- (void) draw;
@end
