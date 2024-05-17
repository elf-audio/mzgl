#pragma once

#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>

#include <memory>

class EventDispatcher;

@interface MZMetalView : MTKView <MTKViewDelegate> {
	std::shared_ptr<EventDispatcher> eventDispatcher;
}
- (id)initWithFrame:(NSRect)frame eventDispatcher:(std::shared_ptr<EventDispatcher>)evtDispatcher;

- (void)disableDrawing;
- (void)enableDrawing;
- (void)shutdown;

- (void)windowResized:(NSNotification *)notification;
- (std::shared_ptr<EventDispatcher>)getEventDispatcher;
@end