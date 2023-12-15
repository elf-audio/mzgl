//
//  EventsView.h
//  mzgl macOS
//
//  Created by Marek Bereza on 03/02/2021.
//  Copyright © 2021 Marek Bereza. All rights reserved.
//

/**
 * DON'T USE THIS USE AppleWebView - eventually delete this and rename AppleWebView as MZGLWebView
 */
#pragma once
#include <TargetConditionals.h>
#include <memory>
class EventDispatcher;
#if !TARGET_OS_IOS
#	import <Cocoa/Cocoa.h>
#endif

#import <WebKit/WebKit.h>

@interface MZGLWebView : WKWebView <WKScriptMessageHandler, WKUIDelegate, NSWindowDelegate>
- (id)initWithFrame:(NSRect)frame
	eventDispatcher:(std::shared_ptr<EventDispatcher>)evtDispatcher
			 andUrl:(NSString *)url;
- (void)shutdown;
// - (void*) getApp;

@end
