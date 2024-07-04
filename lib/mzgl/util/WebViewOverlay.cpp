//
// Created by Marek Bereza on 26/06/2024.
//

#include "WebViewOverlay.h"

class WebViewOverlayImpl {
public:
	WebViewOverlayImpl(App &app, const std::string &url, std::function<void(const std::string &)> jsCallback)
		: app(app)
		, url(url)
		, jsCallback(jsCallback) {}
	virtual ~WebViewOverlayImpl()					 = default;
	virtual void callJs(const std::string &jsString) = 0;

protected:
	const std::string url;
	std::function<void(const std::string &)> jsCallback;
	App &app;
};

///////////////////////////////////////////////////////////////////

#ifdef __APPLE__
#	import <WebKit/WebKit.h>
#	include "AppleWebView.h"

#	include <TargetConditionals.h>
#	if TARGET_OS_IOS
class iOSWebViewOverlayImpl : public WebViewOverlayImpl {
public:
	AppleWebView *webView;
	UIViewController *targetController;

	iOSWebViewOverlayImpl(App &app,
						  const std::string &urlToOpen,
						  std::function<void(const std::string &)> theJsCallback)
		: WebViewOverlayImpl(app, urlToOpen, theJsCallback) {
		targetController = [[UIViewController alloc] init];

		webView					 = [[AppleWebView alloc] initWithFrame:targetController.view.bounds
			 loadedCallback:[]() {}
			 jsCallback:jsCallback
			 closeCallback:[this]() { close(); }
			 url:[NSString stringWithUTF8String:url.c_str()]];
		webView.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;

		[targetController.view addSubview:webView];

		targetController.modalPresentationStyle = UIModalPresentationFormSheet;
		targetController.modalTransitionStyle	= UIModalTransitionStyleFlipHorizontal;

		targetController.view.backgroundColor = [UIColor whiteColor];

		[((__bridge UIViewController *) app.viewController) presentViewController:targetController
																		 animated:YES
																	   completion:nil];
	}
	void callJs(const std::string &jsString) override {
		[webView callJS:[NSString stringWithUTF8String:jsString.c_str()]];
	}
	~iOSWebViewOverlayImpl() = default;

private:
	void close() { [targetController dismissViewControllerAnimated:true completion:nil]; }
};

#	else
class MacWebViewOverlayImpl : public WebViewOverlayImpl {
public:
	AppleWebView *webView;
	MacWebViewOverlayImpl(App &app,
						  const std::string &urlToOpen,
						  std::function<void(const std::string &)> theJsCallback)
		: WebViewOverlayImpl(app, urlToOpen, theJsCallback) {
		dispatch_async(dispatch_get_main_queue(), ^{
		  NSWindow *win	   = (__bridge NSWindow *) app.windowHandle;
		  NSView *rootView = (__bridge NSView *) app.viewHandle;

		  webView				   = [[AppleWebView alloc] initWithFrame:rootView.bounds
			   loadedCallback:[]() {}
			   jsCallback:jsCallback
			   closeCallback:[this]() { close(); }
			   url:[NSString stringWithUTF8String:url.c_str()]];
		  webView.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;

		  [rootView addSubview:webView];
		  animateIn();
		});
	}
	void callJs(const std::string &jsString) override {
		[webView callJS:[NSString stringWithUTF8String:jsString.c_str()]];
	}
	~MacWebViewOverlayImpl() override = default;

private:
	void animateIn() {
		// Initial off-screen position for animation
		NSRect startFrame = webView.frame;
		NSRect endFrame	  = startFrame;
		startFrame.origin.y -= startFrame.size.height;

		webView.frame = startFrame;

		// Animate the web view sliding in
		[NSAnimationContext runAnimationGroup:^(NSAnimationContext *context) {
		  context.duration		 = 0.5;
		  webView.animator.frame = endFrame;
		}];
	}
	void close() {
		// Create slide-out animation
		NSRect endFrame = webView.frame;
		endFrame.origin.y -= webView.bounds.size.height;

		[NSAnimationContext runAnimationGroup:^(NSAnimationContext *context) {
		  context.duration		 = 0.5;
		  webView.animator.frame = endFrame;
		}
			completionHandler:^{ [webView removeFromSuperview]; }];
	}
};
#	endif
#endif
//////////////////////////////////////////////////////////////////

WebViewOverlay::WebViewOverlay(App &app,
							   const std::string &url,
							   std::function<void(const std::string &)> jsCallback) {
#ifdef __APPLE__
#	if TARGET_OS_IOS
	impl = std::make_shared<iOSWebViewOverlayImpl>(app, url, jsCallback);
#	else
	impl = std::make_shared<MacWebViewOverlayImpl>(app, url, jsCallback);
#	endif
#endif
}
void WebViewOverlay::callJs(const std::string &jsString) {
	impl->callJs(jsString);
}
