//
// Created by Marek Bereza on 26/06/2024.
//

#pragma once
#include "App.h"
#include <functional>
#include <memory>

class WebViewOverlayImpl;

class WebViewOverlay {
public:
	WebViewOverlay(
		App &app,
		const std::string &url,
		std::function<void(const std::string &)> jsCallback = [](const std::string &) {},
		const std::string &htmlContent = "");

	void callJs(const std::string &jsString);

	// When true, all navigation stays inside the webview.
	// When false (default), external URLs open in the system browser.
	void setKeepNavigationInternal(bool keep);

	// Called when the webview is closed natively (e.g. closeWindow message).
	void setCloseCallback(std::function<void()> cb);

private:
	std::shared_ptr<WebViewOverlayImpl> impl;
};
