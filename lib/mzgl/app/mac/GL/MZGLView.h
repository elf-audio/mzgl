//
//         _  __                   _ _
//        | |/ _|                 | (_)
//     ___| | |_    __ _ _   _  __| |_  ___
//    / _ \ |  _|  / _` | | | |/ _` | |/ _ \
//   |  __/ | |   | (_| | |_| | (_| | | (_) |
//    \___|_|_|    \__,_|\__,_|\__,_|_|\___/
//
//
//  MZOpenGLView.h - this is the OpenGL View wrapper
//  MZGL
//
//  Created by Marek Bereza on 15/01/2018.
//

#pragma once

#import <Cocoa/Cocoa.h>
#include <memory>

#ifdef USE_METALANGLE
#	import <MetalANGLE/MGLKView.h>
#endif

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

@end
