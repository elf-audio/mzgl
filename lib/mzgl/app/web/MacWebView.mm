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


void MacWebView::show(const std::string &_path, std::function<void()> callbacks) {
	
	auto path = _path;
	dispatch_async(dispatch_get_main_queue(), ^{
		
		NSString *url = [NSString stringWithUTF8String:path.c_str()];

		NSWindow *win = (__bridge NSWindow*)app->windowHandle;
		
		auto cb = [](const std::string &s) {
			printf("js callback: %s\n", s.c_str());
		};
		
		auto closeCb = [this]() {
			close();
		};
		AppleWebView *view = [[AppleWebView alloc] initWithFrame: win.contentView.frame callback:cb closeCallback: closeCb andUrl: url];
		[[win contentView] addSubview:view];
		viewToRemove = (__bridge void*)view;
	});
/*
	// no runloop because no graphics, so lets make one for runOnMainThread
	NSTimer *timer = [NSTimer timerWithTimeInterval:1.0/60.0 repeats:YES block:^(NSTimer * _Nonnull timer) {
		app->updateInternal();
	}];
	
	[[NSRunLoop mainRunLoop] addTimer:timer forMode:NSDefaultRunLoopMode];*/
}

void MacWebView::close() {
	if(viewToRemove!=nullptr) {
		AppleWebView *view = (__bridge AppleWebView *)viewToRemove;
		[view removeFromSuperview];
		viewToRemove = nullptr;
	}
}

void MacWebView::callJS(const std::string &js) {
	
}
