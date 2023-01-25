//
//  MacWebView.mm
//  mzgl
//
//  Created by Marek Bereza on 24/01/2023.
//  Copyright © 2023 Marek Bereza. All rights reserved.
//

#include "MacWebView.h"

#include "MZGLWebView.h"
#include "App.h"
#include "EventDispatcher.h"
#include "AppleWebView.h"

MacWebView::MacWebView(App *app) : app(app) {}


MacWebView::~MacWebView() {}


void MacWebView::show(const std::string &_path,
					  std::function<void(const std::string &data)> jsCallback,
					  std::function<void()> loadedCallback) {
	
	auto path = _path;
	dispatch_async(dispatch_get_main_queue(), ^{
		
		NSString *url = [NSString stringWithUTF8String:path.c_str()];

		NSWindow *win = (__bridge NSWindow*)app->windowHandle;
		
		
		auto closeCb = [this]() {
			close();
		};
		AppleWebView *view = [[AppleWebView alloc] initWithFrame: win.contentView.frame
												  loadedCallback: loadedCallback
													  jsCallback: jsCallback
												   closeCallback: closeCb
															 url: url];
		
		[[win contentView] addSubview:view];
		this->webView = (__bridge void*)view;
	});
/*
	// no runloop because no graphics, so lets make one for runOnMainThread
	NSTimer *timer = [NSTimer timerWithTimeInterval:1.0/60.0 repeats:YES block:^(NSTimer * _Nonnull timer) {
		app->updateInternal();
	}];
	
	[[NSRunLoop mainRunLoop] addTimer:timer forMode:NSDefaultRunLoopMode];*/
}

void MacWebView::close() {
	if(webView!=nullptr) {
		AppleWebView *view = (__bridge AppleWebView *)webView;
		[view removeFromSuperview];
		webView = nullptr;
	}
}

void MacWebView::callJS(const std::string &js) {
	
	NSString *jsStr = [NSString stringWithUTF8String:js.c_str()];
	
	dispatch_async(dispatch_get_main_queue(), ^{
		if(webView!=nullptr) {
			AppleWebView *view = (__bridge AppleWebView *)webView;
			[view callJS:jsStr];
		}
	});
}
