//
//  MZGLES3Renderer.h
//  MZGLiOS
//
//  Created by Marek Bereza on 17/01/2018.
//  Copyright © 2018 Marek Bereza. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <GLKit/GLKit.h>
#include "App.h"

class EventDispatcher;
class Graphics;

@interface MZGLKitView : GLKView
- (std::shared_ptr<App>)getApp;
- (id)initWithApp:(std::shared_ptr<App>)_app andGraphics:(std::shared_ptr<Graphics>)_graphics;
- (std::shared_ptr<EventDispatcher>)getEventDispatcher;
- (BOOL)handleNormalOpen:(NSURL *)url;
- (void) deleteCppObjects;
@end
