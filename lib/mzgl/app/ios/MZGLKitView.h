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

@interface MZGLKitView : GLKView
-(void*) getApp;
- (id) initWithApp:(App*)_app;
-(EventDispatcher*) getEventDispatcher;
-(void) openURLWhenLoadedAndDeleteFile: (NSString*) urlToOpen;
- (BOOL) handleNormalOpen: (NSURL*) url;
@end


