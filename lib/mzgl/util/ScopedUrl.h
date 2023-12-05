//
//  ScopedUrl.h
//  mzgl
//
//  Created by Marek Bereza on 01/12/2023.
//  Copyright © 2023 Marek Bereza. All rights reserved.
//

#pragma once

#include <string>
#include <memory>

class ScopedUrl;
using ScopedUrlRef = std::shared_ptr<ScopedUrl>;

/**
 * This creates a shared_ptr of a url - when it falls out of scope
 * the destructor is called which can delete the file pointed to if needed
 * for things like async loading of a temporary file when you don't want
 * to pass around callbacks.
 */
class ScopedUrl final {
public:
	std::string url;

	/** Creates a url that doesn't get deleted at destruction */
	static ScopedUrlRef create(const std::string &url) {
		return std::shared_ptr<ScopedUrl>(new ScopedUrl(url, false));
	}

	/** Creates a url that DOES get deleted at destruction */
	static ScopedUrlRef createWithDeleter(const std::string &url) {
		return std::shared_ptr<ScopedUrl>(new ScopedUrl(url, true));
	}
	
	static ScopedUrlRef createWithCallback(const std::string &url, std::function<void()> callback) {
		return std::shared_ptr<ScopedUrl>(new ScopedUrl(url, callback));
	}
	void deleteFileOnDestruction() {
		shouldTryToDelete = true;
	}
	// if you want to force things
	// to happen before destructor
	// is called.
	void destructEarly();
	~ScopedUrl();

private:
	bool shouldTryToDelete = false;
	std::function<void()> callback = nullptr;
	
	ScopedUrl(const std::string &url, bool shouldTryToDelete)
		: url(url)
		, shouldTryToDelete(shouldTryToDelete) {}
	
	ScopedUrl(const std::string &url, std::function<void()> callback)
		: url(url)
		, shouldTryToDelete(false)
		, callback(callback) {}
	
};
