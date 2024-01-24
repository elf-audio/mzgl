//
//  AppleWebView.cpp
//  mzgl
//
//  Created by Marek Bereza on 24/01/2023.
//  Copyright © 2023 Marek Bereza. All rights reserved.
//

#include <mzgl/app/apple/AppleWebView.h>

#include <string>
#include <mzgl/util/EventDispatcher.h>
#include <mzgl/util/log.h>
#include <fsystem/fsystem.h>
#include <TargetConditionals.h>

@implementation AppleWebView {
	std::function<void(const std::string &)> jsCallback;
	std::function<void()> closeCallback;
	std::function<void()> loadedCallback;
}

- (void)callJS:(NSString *)jsString {
	[self evaluateJavaScript:jsString
		   completionHandler:^(id _Nullable, NSError *_Nullable error) {
			 NSLog(@"ERRORRRRRR: %@\n\nCall was:\n%@\n", error, jsString);
		   }];
}
- (void)webView:(WKWebView *)webView didFinishNavigation:(WKNavigation *)navigation {
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
	//- (id) initWithFrame:(CGRect)frame callback:(std::function<void(const std::string &)>) jsCb closeCallback:(std::function<void()>)clsCallback andUrl: (NSString*) url {
	loadedCallback				   = loadedCb;
	jsCallback					   = jsCb;
	closeCallback				   = closeCb;
	WKWebViewConfiguration *config = [[WKWebViewConfiguration alloc] init];

	//	[config.userContentController addScriptMessageHandler:self name:@"updateHandler"];
	//	if(window.webkit) {
	//		window.webkit.messageHandlers.updateHandler.postMessage({"key": key, "value": ""+value});
	//	}

	[config.userContentController addScriptMessageHandler:self name:@"closeWindow"];
	[config.userContentController addScriptMessageHandler:self name:@"messages"];
	//	if(window.webkit) {
	//		window.webkit.messageHandlers.updateHandler.postMessage({"key": key, "value": ""+value});
	//	}

	[config.preferences setValue:[NSNumber numberWithBool:YES] forKey:@"fullScreenEnabled"];
	[config.preferences setValue:[NSNumber numberWithBool:YES] forKey:@"DOMPasteAllowed"];
	[config.preferences setValue:[NSNumber numberWithBool:YES] forKey:@"javaScriptCanAccessClipboard"];
#ifdef DEBUG
	//    blah
	[config.preferences setValue:[NSNumber numberWithBool:YES] forKey:@"developerExtrasEnabled"];
#endif

	self				  = [super initWithFrame:frame configuration:config];
	std::string customUrl = [url UTF8String];
	if (self != nil) {
#if !TARGET_OS_IOS
		[self registerForDraggedTypes:[NSArray arrayWithObject:NSFilenamesPboardType]];
#endif
		self.UIDelegate			= self;
		self.navigationDelegate = self;
		if (customUrl != "" && customUrl.find("http") != -1) {
			NSURL *url			  = [NSURL URLWithString:[NSString stringWithFormat:@"%s", customUrl.c_str()]];
			NSURLRequest *request = [NSURLRequest requestWithURL:url];
			[self loadRequest:request];
		} else {
			NSURL *baseURL = [[NSBundle mainBundle] resourceURL];
			baseURL		   = [baseURL URLByAppendingPathComponent:@"www"];
			NSURL *url	   = [baseURL URLByAppendingPathComponent:@"index.html"];

			if (customUrl != "") {
				NSLog(@"Got custom URL!!");
				auto p = fs::path(customUrl);
				if (!fs::exists(p)) {
					NSLog(@"Path does not exist! %s", p.string().c_str());
				}
				baseURL = [NSURL fileURLWithPath:[NSString stringWithUTF8String:p.parent_path().string().c_str()]];
				url		= [baseURL
					URLByAppendingPathComponent:[NSString stringWithUTF8String:p.filename().string().c_str()]];
			}
			NSLog(@"%@", url);

			[self loadFileURL:url allowingReadAccessToURL:baseURL];
		}
	}
	return self;
}
- (void)webView:(WKWebView *)webView
	runJavaScriptAlertPanelWithMessage:(NSString *)message
					  initiatedByFrame:(WKFrameInfo *)frame
					 completionHandler:(void (^)(void))completionHandler {
#if TARGET_OS_IOS
#else
	NSAlert *alert = [[NSAlert alloc] init];
	[alert addButtonWithTitle:@"OK"];
	[alert setMessageText:message];
	[alert setAlertStyle:NSAlertStyleWarning];

	[alert beginSheetModalForWindow:[NSApp mainWindow]
				  completionHandler:^(NSInteger result) { completionHandler(); }];
#endif
}
//
//- (void)windowDidResize:(NSNotification *)notification {
//	Log::d() << "windowDidResize";
//	NSWindow *window = notification.object;
//
//	auto w = window.contentLayoutRect.size.width;
//	auto h = window.contentLayoutRect.size.height;
//
//	Log::d() << w << " " << h;
//
//	NSRect f = self.frame;
//	f.size = CGSizeMake(w, h);;
//	self.frame = f;
//}

- (BOOL)acceptsFirstResponder {
	return YES;
}

- (BOOL)becomeFirstResponder {
	return YES;
}

- (BOOL)isOpaque {
	return YES;
}

- (void)shutdown {
	//	[super shutdown];
	//	eventDispatcher->exit();
}

- (void)userContentController:(nonnull WKUserContentController *)userContentController
	  didReceiveScriptMessage:(nonnull WKScriptMessage *)message {
	if ([message.name isEqual:@"closeWindow"]) {
		if (closeCallback) closeCallback();
		return;
	}
	NSString *str	 = message.body;
	std::string data = [str UTF8String];
	jsCallback(data);
}
@end
