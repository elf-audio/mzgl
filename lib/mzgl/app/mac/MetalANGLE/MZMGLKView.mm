//
//  MZMGLKView.m
//  mzgl macOS
//
//  Created by Marek Bereza on 03/02/2021.
//  Copyright © 2021 Marek Bereza. All rights reserved.
//

#import "MZMGLKView.h"
#include <mutex>
#include "EventDispatcher.h"

@implementation MZMGLKView {
	std::mutex evtMutex;
}


- (void) lock {
	evtMutex.lock();
}

- (void) unlock {
	evtMutex.unlock();
}

- (void) shutdown {
	
}

- (void)windowResized:(NSNotification *)notification {
	
}
- (id) initWithFrame: (NSRect) frame eventDispatcher:(void*)evtDispatcherPtr {
	eventDispatcher = (EventDispatcher*)evtDispatcherPtr;
	self = [super initWithFrame: frame];
	if(self!=nil) {
		// do stuff
	}
	return self;

}
-(void) draw {
//	drawFrame(eventDispatcher->app->g, eventDispatcher);
	if(eventDispatcher->app->g.firstFrame) {
		initMZGL(eventDispatcher->app->g);
		eventDispatcher->setup();
		eventDispatcher->app->g.firstFrame = false;
	}
	eventDispatcher->runFrame();
}
@end
