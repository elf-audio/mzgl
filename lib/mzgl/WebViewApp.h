//
//  WebViewApp.h
//  mzgl
//
//  Created by Marek Bereza on 16/07/2021.
//  Copyright © 2021 Marek Bereza. All rights reserved.
//

#pragma once

#include "App.h"
#include "json.hpp"

class WebViewApp : public App {
public:
	WebViewApp(Graphics &g, const std::string &url = "")
		: App(g)
		, customUrl(url) {}
	bool isWebView() const override { return true; }
	std::string customUrl;
	virtual void receivedJSMessage(const std::string &key, const std::string &value) {}
	std::function<void(const std::string &)> callJS;
	void callJSON(std::string fn, const nlohmann::json &j) {
		auto js = fn + "('" + j.dump() + "')";
		callJS(js);
	}
};
