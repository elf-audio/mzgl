//
//  MZGLWebViewImpl.h
//  mzgl
//
//  Created by Marek Bereza on 24/01/2023.
//  Copyright © 2023 Marek Bereza. All rights reserved.
//

#pragma once
#include <string>
#include <functional>

class MZWebViewImpl {
public:
	
	virtual ~MZWebViewImpl() {}
	/**
	 * Displays a fullscreen web view pointing to the url in the parameter.
	 *
	 * @param path could be a URL (e.g. starting with https://) or a local
	 * file path, which would serve up that folder.
	 *
	 * @param jsCallback this is a callback for javascript calls that get called by
	 * a function that looks like this:
	 *
	 * function sendMessage(msg) {
	 *   if(window.webkit) {
	 *	   window.webkit.messageHandlers.messages.postMessage(msg);
	 * 	 } else {
	 *	   console.log(JSON.stringify(msg));
	 *   }
	 * }
	 *
	 * At the moment I don't know how it's going to work exactly with android
	 *
	 * @param loadedCallback - gets called when WebView is fully loaded and
	 *                         ready to receive javascript calls if needed.
	 */
	virtual void show(const std::string &path,
					  std::function<void(const std::string &data)> jsCallback,
					  std::function<void()> loadedCallback = []() {}) = 0;
	
	/**
	 * Call a javascript function inside the html.
	 */
	virtual void callJS(const std::string &js) = 0;
};
