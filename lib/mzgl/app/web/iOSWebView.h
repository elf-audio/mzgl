//
//  iOSWebView.h
//  mzgl
//
//  Created by Marek Bereza on 24/01/2023.
//  Copyright © 2023 Marek Bereza. All rights reserved.
//

#pragma once

#include "MZWebViewImpl.h"
class App;

class iOSWebView : public MZWebViewImpl {
public:
	iOSWebView(App *app);
	virtual ~iOSWebView();

	void show(const std::string &path, std::function<void()> loadedCallback = []() {}) override;
	
	/**
	 * Call a javascript function inside the html.
	 */
	void callJS(const std::string &js) override;
private:
	App *app = nullptr;
	void *viewController = nullptr;
	void *webView = nullptr;
	void close();
};
