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
#import "GLRenderMode.h"

class GLRenderer;
class App;
class EventDispatcher;

@interface MZGLView : NSOpenGLView {
	std::shared_ptr<EventDispatcher> eventDispatcher;
	std::unique_ptr<GLRenderer> renderer;
}

- (id)initWithFrame:(NSRect)frame
	eventDispatcher:(std::shared_ptr<EventDispatcher>)evtDispatcher
	 withRenderMode:(RenderMode)renderMode;

- (id)initWithFrame:(NSRect)frame eventDispatcher:(std::shared_ptr<EventDispatcher>)evtDispatcher;

- (void)windowResized:(NSNotification *)notification;
- (void)disableDrawing;
- (void)enableDrawing;
- (void)lock;
- (void)unlock;
- (void)shutdown;
- (void)performRender;
- (void)makeContextCurrentForCleanup;
- (std::shared_ptr<App>)getApp;
- (std::shared_ptr<EventDispatcher>)getEventDispatcher;

@property(nonatomic, readwrite, retain) IBOutlet MZGLView *view;

@end
