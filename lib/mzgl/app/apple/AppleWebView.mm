//
//  AppleWebView.cpp
//  mzgl
//
//  Created by Marek Bereza on 24/01/2023.
//  Copyright © 2023 Marek Bereza. All rights reserved.
//

#include "AppleWebView.h"

#include <string>
#include "EventDispatcher.h"
#include "log.h"
#include "filesystem.h"
#include <TargetConditionals.h>
#include "mzgl_platform.h"
@implementation AppleWebView {
	std::function<void(const std::string &)> jsCallback;
	std::function<void()> closeCallback;
	std::function<void()> loadedCallback;
	BOOL hasLoadedInitialPage;
}

- (void)callJS:(NSString *)jsString {
	[self evaluateJavaScript:jsString
		   completionHandler:^(id _Nullable, NSError *_Nullable error) {
			 if (error) {
				 NSLog(@"ERROR: %@\n\nCall was:\n%@\n", error, jsString);
			 }
		   }];
}
- (void)webView:(WKWebView *)webView didFinishNavigation:(WKNavigation *)navigation {
	hasLoadedInitialPage = YES;
	if (loadedCallback) {
		loadedCallback();

		// only call loadCallback once!
		loadedCallback = nullptr;
	}
}

- (id)initWithFrame:(CGRect)frame
	 loadedCallback:(std::function<void()>)loadedCb
		 jsCallback:(std::function<void(const std::string &)>)jsCb
	  closeCallback:(std::function<void()>)closeCb
				url:(NSString *)url {
	return [self initWithFrame:frame
				loadedCallback:loadedCb
					jsCallback:jsCb
				 closeCallback:closeCb
						   url:url
				   htmlContent:nil
					   baseURL:nil];
}

- (id)initWithFrame:(CGRect)frame
	 loadedCallback:(std::function<void()>)loadedCb
		 jsCallback:(std::function<void(const std::string &)>)jsCb
	  closeCallback:(std::function<void()>)closeCb
				url:(NSString *)url
		htmlContent:(NSString *)htmlContent
			baseURL:(NSString *)baseURL {
	loadedCallback				   = loadedCb;
	jsCallback					   = jsCb;
	closeCallback				   = closeCb;
	hasLoadedInitialPage		   = NO;
	WKWebViewConfiguration *config = [[WKWebViewConfiguration alloc] init];

	[config.userContentController addScriptMessageHandler:self name:@"closeWindow"];
	[config.userContentController addScriptMessageHandler:self name:@"messages"];

	[config.preferences setValue:[NSNumber numberWithBool:YES] forKey:@"fullScreenEnabled"];
	[config.preferences setValue:[NSNumber numberWithBool:YES] forKey:@"DOMPasteAllowed"];
	[config.preferences setValue:[NSNumber numberWithBool:YES] forKey:@"javaScriptCanAccessClipboard"];
#ifdef DEBUG
	[config.preferences setValue:[NSNumber numberWithBool:YES] forKey:@"developerExtrasEnabled"];
#endif

	self = [super initWithFrame:frame configuration:config];

	if (self != nil) {
#if !MZGL_IOS
		[self registerForDraggedTypes:@[ NSPasteboardTypeFileURL ]];
#endif
		self.UIDelegate			= self;
		self.navigationDelegate = self;
		
		// If HTML content is provided, load it with the base URL
		if (htmlContent != nil && baseURL != nil) {
			NSLog(@"Loading HTML with base URL: %@", baseURL);
			[self loadHTMLString:htmlContent baseURL:[NSURL URLWithString:baseURL]];
		} else if ([url rangeOfString:@"http"].location != NSNotFound) {
			NSURLRequest *request = [NSURLRequest requestWithURL:[NSURL URLWithString:url]];
			[self loadRequest:request];
		} else {
			NSURL *nsUrl = [[[NSBundle mainBundle] resourceURL] URLByAppendingPathComponent:@"www/index.html"];

			if ([url length] > 0) {
				if ([url containsString:@"://"]) {
					nsUrl = [NSURL URLWithString:url];
				} else {
					nsUrl = [NSURL fileURLWithPath:url];
				}
			}
			NSLog(@"Opening URL: %@", nsUrl);

			[self loadFileURL:nsUrl allowingReadAccessToURL:[nsUrl URLByDeletingLastPathComponent]];
		}
	}
	return self;
}
- (void)webView:(WKWebView *)webView
	runJavaScriptAlertPanelWithMessage:(NSString *)message
					  initiatedByFrame:(WKFrameInfo *)frame
					 completionHandler:(void (^)(void))completionHandler {
#if MZGL_IOS
#else
	NSAlert *alert = [[NSAlert alloc] init];
	[alert addButtonWithTitle:@"OK"];
	[alert setMessageText:message];
	[alert setAlertStyle:NSAlertStyleWarning];

	[alert beginSheetModalForWindow:[NSApp mainWindow]
				  completionHandler:^(NSInteger result) { completionHandler(); }];
#endif
}

- (BOOL)acceptsFirstResponder {
	return YES;
}

- (BOOL)becomeFirstResponder {
	return [super becomeFirstResponder];
}

- (BOOL)isOpaque {
	return YES;
}

- (void)shutdown {
	//	[super shutdown];
	//	eventDispatcher->exit();
}

- (void)webView:(WKWebView *)webView
	decidePolicyForNavigationAction:(WKNavigationAction *)navigationAction
					decisionHandler:(void (^)(WKNavigationActionPolicy))decisionHandler {
	NSLog(@"Navigation action: %@", navigationAction.request.URL);
	NSURL *url = navigationAction.request.URL;

	// Allow the initial page load (needed for loadHTMLString:baseURL: with https base URL)
	if (!hasLoadedInitialPage) {
		decisionHandler(WKNavigationActionPolicyAllow);
		return;
	}

	// When keepNavigationInternal is set, all navigation stays in the webview
	if (self.keepNavigationInternal) {
		decisionHandler(WKNavigationActionPolicyAllow);
		return;
	}

	// Default: file/localhost allowed, everything else opens in system browser
	BOOL isInternal = [url.scheme isEqual:@"file"] || [url.host isEqual:@"localhost"];

	if (!isInternal) {
		if (url == nil) return;
		launchUrl([[url absoluteString] UTF8String]);
		decisionHandler(WKNavigationActionPolicyCancel);
	} else {
		decisionHandler(WKNavigationActionPolicyAllow);
	}
}

- (void)userContentController:(nonnull WKUserContentController *)userContentController
	  didReceiveScriptMessage:(nonnull WKScriptMessage *)message {
	if ([message.name isEqual:@"closeWindow"] || [message.body isEqual:@"close"]) {
		if (closeCallback) closeCallback();
		return;
	}
	NSString *str	 = message.body;
	std::string data = [str UTF8String];
	jsCallback(data);
}
@end
