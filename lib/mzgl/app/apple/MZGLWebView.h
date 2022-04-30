//
//  EventsView.h
//  mzgl macOS
//
//  Created by Marek Bereza on 03/02/2021.
//  Copyright © 2021 Marek Bereza. All rights reserved.
//

#pragma once
#include <TargetConditionals.h>

#if !TARGET_OS_IOS
#import <Cocoa/Cocoa.h>
#endif


#import <WebKit/WebKit.h>

@interface MZGLWebView : WKWebView<WKScriptMessageHandler, WKUIDelegate, NSWindowDelegate>
- (id) initWithFrame: (NSRect) frame eventDispatcher:(void*)evtDispatcherPtr;
- (void) shutdown;
- (void*) getApp;

@end

