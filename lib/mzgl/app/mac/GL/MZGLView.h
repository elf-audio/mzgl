//
//  MZOpenGLView.h - this is the OpenGL View wrapper
//  MZGL
//
//  Created by Marek Bereza on 15/01/2018.
//  Copyright © 2018 Marek Bereza. All rights reserved.
//

#pragma once

#import <Cocoa/Cocoa.h>
#include <memory>

//#include "App.h"
class EffectPlugin;

class App;
class EventDispatcher;
@interface MZGLView : NSOpenGLView {
	std::shared_ptr<EventDispatcher> eventDispatcher;
}

- (id)initWithFrame:(NSRect)frame eventDispatcher:(std::shared_ptr<EventDispatcher>)evtDispatcher;
- (void)windowResized:(NSNotification *)notification;
- (void)disableDrawing;
- (void)enableDrawing;
- (void)lock;
- (void)unlock;
- (void)shutdown;
- (std::shared_ptr<App>)getApp;
- (std::shared_ptr<EventDispatcher>)getEventDispatcher;

@property(nonatomic, readwrite, retain) IBOutlet MZGLView *view;

@end
