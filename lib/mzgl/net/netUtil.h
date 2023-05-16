//
//  netUtil.h
//  mzgl
//
//  Created by Marek Bereza on 16/05/2023.
//  Copyright © 2023 Marek Bereza. All rights reserved.
//

#pragma once

#include <string>
class DownloadError : public std::runtime_error {
public:
	DownloadError(const std::string &url, const std::string &message)
		: std::runtime_error(message), url(url) {}
	std::string url;
};

// throws DownloadError
std::string downloadUrl(std::string url);
