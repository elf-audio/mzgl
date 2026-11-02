//
//  LinuxWebView.h
//  mzgl
//
//  WebKitGTK web view for Linux - the equivalent of the WKWebView / WebView2
//  overlays on the other platforms. GLFW's window isn't a GTK widget (and on
//  Wayland a foreign surface can't be embedded in it anyway), so the page opens
//  in its own GTK top-level window instead of covering the app's.
//
//  libwebkit2gtk-4.1 is dlopen()ed on first use rather than linked, so the app
//  still starts on machines without it; there the page opens in the system
//  browser instead (without the javascript bridge). GTK's main loop is pumped
//  from the app's main-thread queue for as long as the window is open.
//

#pragma once

#include <functional>
#include <memory>
#include <string>

class App;
class LinuxWebViewImpl;

class LinuxWebView {
public:
	struct Options {
		// http(s) url, or a local file path, to load.
		std::string url;

		// When true, navigating away from the initial page (other than file://
		// and localhost) is cancelled and opened in the system browser instead,
		// mirroring OpenLinksInSafariDelegate / AppleWebView on Apple.
		bool openLinksInBrowser = false;

		int width  = 960;
		int height = 720;

		// Messages posted from the page via sendMessage() in www/js/utils.js
		// (window.webkit.messageHandlers.messages, which WebKitGTK provides
		// natively). A "close" message closes the window instead.
		std::function<void(const std::string &)> jsCallback;

		// Fired once the window has gone, whether the user closed it, the page
		// posted "close", or close() was called.
		std::function<void()> onClosed;
	};

	// Returns false if WebKitGTK couldn't be loaded (or GTK can't start).
	static bool isAvailable();

	LinuxWebView(App &app, Options options);

	// Closes the window if it's still open.
	~LinuxWebView();

	void callJs(const std::string &js);
	void close();

private:
	std::shared_ptr<LinuxWebViewImpl> impl;
};
