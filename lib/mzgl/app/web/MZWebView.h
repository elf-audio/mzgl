//
//  MZWebView.h
//  mzgl
//
//  Created by Marek Bereza on 24/01/2023.
//  Copyright © 2023 Marek Bereza. All rights reserved.
//

#pragma once

#include <memory>
#include <string>
#include <functional>

class MZWebViewImpl;
class App;

// generic wrapper for a webview, eventually MZGLWebView will merge into this.
class MZWebView {
public:
	MZWebView(App *app);
	
	/**
	 * Displays a fullscreen web view pointing to the url in the parameter.
	 *
	 * @param path could be a URL (e.g. starting with https://) or a local
	 * file path, which would serve up that folder.
	 */
	void show(const std::string &path, std::function<void()> loadedCallback = []() {});
	
	/**
	 * Call a javascript function inside the html.
	 */
	void callJS(const std::string &js);
	
	
	
	std::shared_ptr<MZWebViewImpl> impl;
};
