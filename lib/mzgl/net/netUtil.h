//
//  netUtil.h
//  mzgl
//
//  Created by Marek Bereza on 16/05/2023.
//  Copyright © 2023 Marek Bereza. All rights reserved.
//

#pragma once

#include <stdexcept>
#include <string>
#include <vector>

class DownloadError : public std::runtime_error {
public:
	DownloadError(const std::string &url, const std::string &message)
		: std::runtime_error(message)
		, url(url) {}
	std::string url;
};

// throws DownloadError
std::string downloadUrl(std::string url);

std::string postToUrl(const std::string url, const std::vector<std::pair<std::string, std::string>> &params);
void pingUrl(const std::string &url);
std::string urlencode(const std::string &value);
std::string urldecode(const std::string &value);
// /Users/marek/test files -> file:///Users/marek/test%20files
std::string pathToFileUrl(const std::string &path);