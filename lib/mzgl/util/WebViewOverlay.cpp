//
// Created by Marek Bereza on 26/06/2024.
//

#include "WebViewOverlay.h"

class WebViewOverlayImpl {
public:
	WebViewOverlayImpl(App &app, 
					   const std::string &url, 
					   std::function<void(const std::string &)> jsCallback,
					   const std::string &htmlContent)
		: app(app)
		, url(url)
		, jsCallback(jsCallback)
		, htmlContent(htmlContent) {}

	virtual ~WebViewOverlayImpl() = default;

	virtual void callJs(const std::string &jsString) = 0;
	virtual void setKeepNavigationInternal(bool keep) { keepInternal = keep; }

	std::function<void()> closeCallback;

protected:
	const std::string url;
	std::function<void(const std::string &)> jsCallback;
	App &app;
	const std::string htmlContent;
	bool keepInternal = false;
};

///////////////////////////////////////////////////////////////////

#ifdef __APPLE__
#	include "AppleWebView.h"
#	import <WebKit/WebKit.h>
#	include <TargetConditionals.h>
#	if TARGET_OS_IOS
class iOSWebViewOverlayImpl : public WebViewOverlayImpl {
public:
	AppleWebView *webView;
	UIViewController *targetController;

	iOSWebViewOverlayImpl(App &app,
						  const std::string &urlToOpen,
						  std::function<void(const std::string &)> theJsCallback,
						  const std::string &htmlContent)
		: WebViewOverlayImpl(app, urlToOpen, theJsCallback, htmlContent) {
		targetController = [[UIViewController alloc] init];

		NSString *nsUrl = [NSString stringWithUTF8String:url.c_str()];
		NSString *nsHtml = htmlContent.empty() ? nil : [NSString stringWithUTF8String:htmlContent.c_str()];

		webView = [[AppleWebView alloc] initWithFrame:targetController.view.bounds
						   loadedCallback:[]() {}
							   jsCallback:jsCallback
							closeCallback:[this]() { close(); }
									  url:nsUrl
							  htmlContent:nsHtml
								  baseURL:nsUrl];
		webView.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
		webView.keepNavigationInternal = keepInternal ? YES : NO;

		[targetController.view addSubview:webView];

		targetController.modalPresentationStyle = UIModalPresentationFullScreen;
		targetController.modalTransitionStyle	= UIModalTransitionStyleFlipHorizontal;

		targetController.view.backgroundColor = [UIColor whiteColor];

		[((__bridge UIViewController *) app.viewController) presentViewController:targetController
																		 animated:YES
																	   completion:nil];
	}
	void callJs(const std::string &jsString) override {
		[webView callJS:[NSString stringWithUTF8String:jsString.c_str()]];
	}
	void setKeepNavigationInternal(bool keep) override {
		keepInternal = keep;
		if (webView) {
			webView.keepNavigationInternal = keep ? YES : NO;
		}
	}
	~iOSWebViewOverlayImpl() = default;

private:
	void close() {
		[targetController dismissViewControllerAnimated:true completion:nil];
		if (closeCallback) closeCallback();
	}
};

#	else
class MacWebViewOverlayImpl : public WebViewOverlayImpl {
public:
	AppleWebView *webView;
	MacWebViewOverlayImpl(App &app,
						  const std::string &urlToOpen,
						  std::function<void(const std::string &)> theJsCallback,
						  const std::string &htmlContent)
		: WebViewOverlayImpl(app, urlToOpen, theJsCallback, htmlContent) {
		dispatch_async(dispatch_get_main_queue(), ^{
		  NSView *rootView = (__bridge NSView *) app.viewHandle;

		  NSString *nsUrl = [NSString stringWithUTF8String:url.c_str()];
		  NSString *nsHtml = this->htmlContent.empty() ? nil : [NSString stringWithUTF8String:this->htmlContent.c_str()];
		  
		  webView = [[AppleWebView alloc] initWithFrame:rootView.bounds
							 loadedCallback:[]() {}
								 jsCallback:jsCallback
							  closeCallback:[this]() { close(); }
										url:nsUrl
								htmlContent:nsHtml
									baseURL:nsUrl];
		  webView.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
		  webView.keepNavigationInternal = this->keepInternal ? YES : NO;

		  [rootView addSubview:webView];
		  animateIn();
		});
	}
	void callJs(const std::string &jsString) override {
		[webView callJS:[NSString stringWithUTF8String:jsString.c_str()]];
	}
	void setKeepNavigationInternal(bool keep) override {
		keepInternal = keep;
		if (webView) {
			webView.keepNavigationInternal = keep ? YES : NO;
		}
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
		NSView *rootView = webView.superview;

		[NSAnimationContext runAnimationGroup:^(NSAnimationContext *context) {
		  context.duration		 = 0.5;
		  webView.animator.frame = endFrame;
		}
			completionHandler:^{
			  [webView removeFromSuperview];
			  [rootView.window makeFirstResponder:rootView];
			}];
		if (closeCallback) closeCallback();
	}
};
#	endif
#elif defined(__ANDROID__)

#	include "androidUtil.h"
#	include <fstream>

class AndroidWebViewOverlayImpl : public WebViewOverlayImpl {
public:
	AndroidWebViewOverlayImpl(App &app,
							  const std::string &urlToOpen,
							  std::function<void(const std::string &)> theJsCallback,
							  const std::string &htmlContent)
		: WebViewOverlayImpl(app, urlToOpen, theJsCallback, htmlContent) {
		registerWebViewOverlay(reinterpret_cast<std::uintptr_t>(this), theJsCallback);
		readHtml(urlToOpen);
	}

	~AndroidWebViewOverlayImpl() override {
		androidStopDisplayingHtml();
		unregisterWebViewOverlay(reinterpret_cast<std::uintptr_t>(this));
	}

	void callJs(const std::string &jsString) override { androidCallJs(jsString); }

private:
	void readHtml(const std::string &urlToOpen) {
		if (startsWith(urlToOpen, "file:///android_asset", CaseSensitivity::caseInSensitive)
			|| startsWith(urlToOpen, "https://www.", CaseSensitivity::caseInSensitive)) {
			Log::d() << "Android, opening " << urlToOpen << " as file";
			androidDisplayHtmlFile(urlToOpen);
			return;
		}

		Log::d() << "Android, opening " << urlToOpen << " as stream buffer";
		std::ifstream inputStream(urlToOpen);
		if (!inputStream.is_open()) {
			Log::e() << "Failed to open Html on path " << url;
			return;
		}
		std::stringstream buffer;
		buffer << inputStream.rdbuf();
		androidDisplayHtml(buffer.str());
	}
};

#endif

WebViewOverlay::WebViewOverlay(App &app,
							   const std::string &url,
							   std::function<void(const std::string &)> jsCallback,
							   const std::string &htmlContent) {
#ifdef __APPLE__
#	if TARGET_OS_IOS
	impl = std::make_shared<iOSWebViewOverlayImpl>(app, url, jsCallback, htmlContent);
#	else
	impl = std::make_shared<MacWebViewOverlayImpl>(app, url, jsCallback, htmlContent);
#	endif
#elif defined(__ANDROID__)
	impl = std::make_shared<AndroidWebViewOverlayImpl>(app, url, jsCallback, htmlContent);
#endif
}

void WebViewOverlay::callJs(const std::string &jsString) {
	impl->callJs(jsString);
}

void WebViewOverlay::setKeepNavigationInternal(bool keep) {
	impl->setKeepNavigationInternal(keep);
}

void WebViewOverlay::setCloseCallback(std::function<void()> cb) {
	impl->closeCallback = std::move(cb);
}
