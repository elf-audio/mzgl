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
	 */
	virtual void show(const std::string &path, std::function<void()> callbacks = []() {}) = 0;
	
	/**
	 * Call a javascript function inside the html.
	 */
	virtual void callJS(const std::string &js) = 0;
};
