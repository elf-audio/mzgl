//
//  iOSWebView.cpp
//  mzgl
//
//  Created by Marek Bereza on 24/01/2023.
//  Copyright © 2023 Marek Bereza. All rights reserved.
//

#include "iOSWebView.h"

#include "App.h"
#include "EventDispatcher.h"
#include "AppleWebView.h"

iOSWebView::iOSWebView(App *app) : app(app) {}


iOSWebView::~iOSWebView() {}


void iOSWebView::show(const std::string &_path,
					  std::function<void(const std::string &data)> jsCallback,
					  std::function<void()> loadedCallback) {
	std::string path = _path;
	dispatch_async(dispatch_get_main_queue(), ^{
		
		NSString *url = [NSString stringWithUTF8String:path.c_str()];
//		url = @"https://www.google.com";
		UIWindow *win = (__bridge UIWindow*)app->windowHandle;
		
		
//		CGRect rect = CGRectMake(0, 0, 100, 100);
//		AppleWebView *view = [[AppleWebView alloc] initWithFrame: rect eventDispatcher: &evts andUrl: url];
//		[[win contentView] addSubview:view];
		
	
		auto closeCb = [this]() {
			close();
		};
		AppleWebView *view = [[AppleWebView alloc] initWithFrame: win.bounds
												  loadedCallback: loadedCallback
													  jsCallback: jsCallback
												   closeCallback: closeCb
															 url: url];
		webView = (__bridge void*)view;
		
		
		UIViewController *targetController = [[UIViewController alloc] init];

		[targetController.view addSubview:view];
		
		targetController.modalPresentationStyle = UIModalPresentationFullScreen;
		targetController.modalTransitionStyle = UIModalTransitionStyleFlipHorizontal;
		targetController.view.backgroundColor = [UIColor whiteColor];
		NSDictionary *views = NSDictionaryOfVariableBindings(view);
		[targetController.view addConstraints:[NSLayoutConstraint constraintsWithVisualFormat:@"H:|-0-[view]-0-|" options:0 metrics:nil views:views]];
		[targetController.view addConstraints:[NSLayoutConstraint constraintsWithVisualFormat:@"V:|-0-[view]-0-|" options:0 metrics:nil views:views]];


		[((__bridge UIViewController*)app->viewController) presentViewController:targetController animated:YES completion:nil];
		
		viewController = (__bridge void*)targetController;
		
	});
/*
	// no runloop because no graphics, so lets make one for runOnMainThread
	NSTimer *timer = [NSTimer timerWithTimeInterval:1.0/60.0 repeats:YES block:^(NSTimer * _Nonnull timer) {
		app->updateInternal();
	}];
	
	[[NSRunLoop mainRunLoop] addTimer:timer forMode:NSDefaultRunLoopMode];*/
}

void iOSWebView::callJS(const std::string &js) {
	NSString *jsStr = [NSString stringWithUTF8String:js.c_str()];
	
	if(webView!=nullptr) {
		AppleWebView *view = (__bridge AppleWebView *)webView;
		[view callJS:jsStr];
	}
}


void iOSWebView::close() {
	if(viewController!=nullptr) {
		UIViewController *targetController = (__bridge UIViewController*)viewController;
		[targetController dismissViewControllerAnimated:true completion:nil];
		viewController = nullptr;
		webView = nullptr;
	}
}


