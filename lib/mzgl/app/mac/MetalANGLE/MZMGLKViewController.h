//
//  MZMGLKViewController.h
//  metalbarebones
//
//  Created by Marek Bereza on 02/02/2021.
//

#import <Cocoa/Cocoa.h>

#import <MetalANGLE/MGLKViewController.h>

@interface MZMGLKViewController : MGLKViewController<MGLKViewDelegate>
- (id) initWithFrame: (NSRect) frame eventDispatcher:(void*)evtDispatcherPtr;

@end
