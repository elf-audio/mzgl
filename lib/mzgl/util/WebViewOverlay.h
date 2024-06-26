//
// Created by Marek Bereza on 26/06/2024.
//

#pragma once
#include "App.h"
#include <memory>

class WebViewOverlayImpl;

class WebViewOverlay {
public:
	WebViewOverlay(
		App &app,
		const std::string &url,
		std::function<void(const std::string &)> jsCallback = [](const std::string &) {});

	void callJs(const std::string &jsString);

private:
	std::shared_ptr<WebViewOverlayImpl> impl;
};
