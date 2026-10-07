//
//  WindowsWebView.h
//  mzgl
//
//  Embedded WebView2 (Edge/Chromium) overlay for Windows - the equivalent of the
//  WKWebView overlays on mac/iOS. It covers the app window's client area with a
//  close-button bar along the top and the web view underneath. Everything runs on
//  the main thread; WebView2's async callbacks are dispatched by the app's normal
//  message pump (glfwPollEvents).
//

#pragma once

#include <functional>
#include <memory>
#include <string>

class App;
class WindowsWebViewImpl;

class WindowsWebView {
public:
	struct Options {
		// http(s) url, or a local file path, to load. Ignored when html is set.
		std::string url;

		// Raw html to display instead of navigating to url.
		std::string html;

		// When true, any navigation away from the initial page (other than file://
		// and localhost) is cancelled and opened in the system browser instead,
		// mirroring OpenLinksInSafariDelegate / AppleWebView on Apple.
		bool openLinksInBrowser = false;

		// Navigations with this scheme (e.g. "koala") are cancelled and routed to
		// App::openUrl in-process - see KoalaSchemeNavDelegate in Dialogs.cpp.
		std::string appUrlScheme;

		// Native bar with a close button above the page. Turn off for pages that
		// have their own close control (they post "close" - see jsCallback).
		bool showCloseBar = true;

		// Appended to the User-Agent so sites can tell they're inside the app.
		std::string userAgentSuffix;

		// Messages posted from the page via sendMessage() in www/js/utils.js. The
		// page's window.webkit.messageHandlers shim is installed automatically so
		// the same html works here as on Apple. A "close" message closes the view.
		std::function<void(const std::string &)> jsCallback;

		// Fired once the overlay has been torn down, whether via the close
		// button, a "close" message, or close().
		std::function<void()> onClosed;
	};

	WindowsWebView(App &app, Options options);

	// Closes the overlay if it's still open.
	~WindowsWebView();

	void callJs(const std::string &js);
	void close();

private:
	std::shared_ptr<WindowsWebViewImpl> impl;
};
