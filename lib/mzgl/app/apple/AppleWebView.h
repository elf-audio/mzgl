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

@interface AppleWebView : WKWebView<WKScriptMessageHandler, WKUIDelegate, WKNavigationDelegate>
- (id) initWithFrame: (CGRect)frame
	  loadedCallback: (std::function<void()>) loadedCallback
		  jsCallback: (std::function<void(const std::string &)>) jsCb
	   closeCallback: (std::function<void()>)closeCallback
				 url: (NSString*) url;

- (void) callJS:(NSString*) jsString;
@end

