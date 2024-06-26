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
	virtual ~WebViewOverlayImpl() {}
	virtual void callJs(const std::string &jsString) = 0;

protected:
	std::string url;
	std::function<void(const std::string &)> jsCallback;
	App &app;
};

///////////////////////////////////////////////////////////////////
#include "NSBlockButton.h"
#import <WebKit/WebKit.h>
#include "AppleWebView.h"
//#import <Cocoa/Cocoa.h>
class MacWebViewOverlayImpl : public WebViewOverlayImpl {
public:
	NSView *containerView;
	AppleWebView *webView;
	MacWebViewOverlayImpl(App &app,
						  const std::string &urlToOpen,
						  std::function<void(const std::string &)> theJsCallback)
		: WebViewOverlayImpl(app, urlToOpen, theJsCallback) {
		dispatch_async(dispatch_get_main_queue(), ^{
		  NSWindow *win	   = (__bridge NSWindow *) app.windowHandle;
		  NSView *rootView = (__bridge NSView *) app.viewHandle;

		  containerView = [[NSView alloc] initWithFrame:rootView.bounds];
		  // Make contentView background black
		  containerView.wantsLayer			  = YES;
		  containerView.layer.backgroundColor = [NSColor blackColor].CGColor;
		  containerView.autoresizingMask	  = NSViewWidthSizable | NSViewHeightSizable;

		  int buttonHeight = 20;
		  int padding	   = 10;

		  NSRect wvRect = NSMakeRect(
			  0, 0, rootView.bounds.size.width, rootView.bounds.size.height - buttonHeight - padding * 2);

		  // Instantiate WKWebView on the main thread
		  webView				   = [[AppleWebView alloc] initWithFrame:wvRect
			   loadedCallback:[]() {}
			   jsCallback:jsCallback
			   closeCallback:[]() {}
			   url:[NSString stringWithUTF8String:url.c_str()]];
		  webView.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;

		  // Create and configure the close button
		  NSBlockButton *closeButton =
			  [[NSBlockButton alloc] initWithFrame:NSMakeRect(rootView.bounds.size.width - 30,
															  rootView.bounds.size.height - buttonHeight - padding,
															  buttonHeight,
															  buttonHeight)];
		  closeButton.title = @"✕";
		  [closeButton setButtonType:NSButtonTypeMomentaryPushIn];
		  [closeButton setBezelStyle:NSBezelStyleInline];
		  closeButton.font			   = [NSFont systemFontOfSize:16];
		  closeButton.bordered		   = NO;
		  closeButton.autoresizingMask = NSViewMinXMargin | NSViewMinYMargin;

		  __weak NSView *weakContainerView = containerView;
		  [closeButton setActionBlock:^{ close(); }];
		  [containerView addSubview:webView];
		  [containerView addSubview:closeButton];
		  [rootView addSubview:containerView];

		  animateIn();
		});
	}
	void callJs(const std::string &jsString) override {
		[webView callJS:[NSString stringWithUTF8String:jsString.c_str()]];
	}
	~MacWebViewOverlayImpl() = default;

private:
	void animateIn() {
		// Initial off-screen position for animation
		NSRect startFrame = containerView.frame;
		NSRect endFrame	  = startFrame;
		startFrame.origin.y -= startFrame.size.height;

		containerView.frame = startFrame;

		// Animate the web view sliding in
		[NSAnimationContext runAnimationGroup:^(NSAnimationContext *context) {
		  context.duration			   = 0.5;
		  containerView.animator.frame = endFrame;
		}];
	}
	void close() {
		// Create slide-out animation
		NSRect endFrame = containerView.frame;
		endFrame.origin.y -= containerView.bounds.size.height;

		[NSAnimationContext runAnimationGroup:^(NSAnimationContext *context) {
		  context.duration			   = 0.5;
		  containerView.animator.frame = endFrame;
		}
			completionHandler:^{ [containerView removeFromSuperview]; }];
	}
};

//////////////////////////////////////////////////////////////////

WebViewOverlay::WebViewOverlay(App &app,
							   const std::string &url,
							   std::function<void(const std::string &)> jsCallback) {
	impl = std::make_shared<MacWebViewOverlayImpl>(app, url, jsCallback);
}
void WebViewOverlay::callJs(const std::string &jsString) {
	impl->callJs(jsString);
}