//
//  MacWebView.h
//  mzgl
//
//  Created by Marek Bereza on 24/01/2023.
//  Copyright © 2023 Marek Bereza. All rights reserved.
//

#pragma once

#include "MZWebViewImpl.h"
class App;

class AndroidWebView : public MZWebViewImpl {
public:
	AndroidWebView(App *app);
	virtual ~AndroidWebView();

	void show(const std::string &path, std::function<void()> callbacks = []() {}) override;
	
	/**
	 * Call a javascript function inside the html.
	 */
	void callJS(const std::string &js) override;
private:
	void close();
	App *app = nullptr;

};
