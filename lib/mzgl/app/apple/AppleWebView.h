//
//  AppleWebView.h
//  mzgl
//
//  Created by Marek Bereza on 24/01/2023.
//  Copyright © 2023 Marek Bereza. All rights reserved.
//


/**
 * This is the new MZGLWebView
 */
#pragma once
#include <TargetConditionals.h>

#if !TARGET_OS_IOS
#import <Cocoa/Cocoa.h>
#endif


#import <WebKit/WebKit.h>
#include <string>
#include <functional>

@interface AppleWebView : WKWebView<WKScriptMessageHandler, WKUIDelegate>
- (id) initWithFrame:(CGRect)frame callback:(std::function<void(const std::string &)>) jsCb closeCallback:(std::function<void()>)closeCallback andUrl: (NSString*) url;
- (void) callJS:(const char *) jsString;
@end

