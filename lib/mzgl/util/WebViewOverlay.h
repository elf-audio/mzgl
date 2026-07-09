//
// Created by Marek Bereza on 26/06/2024.
//

#pragma once
#include "App.h"
#include <memory>

class WebViewOverlayImpl;

class WebViewOverlay {
public:
	// onClosed fires when the user dismisses the overlay natively (Apple's
	// close button) - lets the owner stop routing messages into a dead view.
	// On Android the close arrives as a "close" string through jsCallback
	// instead.
	WebViewOverlay(
		App &app,
		const std::string &url,
		std::function<void(const std::string &)> jsCallback = [](const std::string &) {},
		std::function<void()> onClosed						= nullptr);

	void callJs(const std::string &jsString);

private:
	std::shared_ptr<WebViewOverlayImpl> impl;
};
