//
//  netUtil.cpp
//  mzgl
//
//  Created by Marek Bereza on 16/05/2023.
//  Copyright © 2023 Marek Bereza. All rights reserved.
//

#include "netUtil.h"
#include "stringUtil.h"
#include "util.h"
#include "log.h"
#include "mzgl_platform.h"

#ifdef __APPLE__
#	include <Foundation/Foundation.h>
#endif

#if defined(__linux__)
#	include <array>
#	include <cstdio>
#endif

#ifdef _WIN32
#	ifndef WIN32_LEAN_AND_MEAN
#		define WIN32_LEAN_AND_MEAN
#	endif
#	ifndef NOMINMAX
#		define NOMINMAX
#	endif
#	include <windows.h>
#	include <winhttp.h>
#	include "winUtil.h"
#	pragma comment(lib, "winhttp.lib")
#endif

std::string downloadUrl(std::string url) {
#ifdef __APPLE__
	// the URL to save
	NSURL *urlNS = [NSURL URLWithString:[NSString stringWithUTF8String:url.c_str()]];

	NSError *error = nil;
	// turn it into a request and use NSData to load its content
	//	NSURLRequest *request = [NSURLRequest requestWithURL:urlNS];
	//	NSData *data = [NSURLConnection sendSynchronousRequest:request returningResponse:nil error:nil];
	NSData *data = [NSData dataWithContentsOfURL:urlNS options:NSDataReadingUncached error:&error];

	if (error != nil) {
		NSLog(@"Error downloading data: %@", error);
		std::string errorMessage = [NSString stringWithFormat:@"Error downloading data: %@", error].UTF8String;
		throw DownloadError(url, errorMessage);
	}

	NSString *str = [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];
	if (str == nil) {
		throw DownloadError(url, "Couldn't convert NSData to string in downloadUrl");
	}
	return [str UTF8String];
#elif defined(_WIN32)
	std::wstring wurl = n2w(url);
	URL_COMPONENTSW comps {};
	comps.dwStructSize		= sizeof(comps);
	wchar_t host[256]		= {};
	wchar_t path[2048]		= {};
	comps.lpszHostName		= host;
	comps.dwHostNameLength	= ARRAYSIZE(host);
	comps.lpszUrlPath		= path;
	comps.dwUrlPathLength	= ARRAYSIZE(path);
	if (!WinHttpCrackUrl(wurl.c_str(), 0, 0, &comps)) {
		throw DownloadError(url, "Could not parse URL");
	}

	HINTERNET hSession = WinHttpOpen(
		L"mzgl/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
	if (!hSession) throw DownloadError(url, "WinHttpOpen failed");

	HINTERNET hConnect = WinHttpConnect(hSession, host, comps.nPort, 0);
	HINTERNET hRequest = nullptr;
	if (hConnect) {
		DWORD flags = (comps.nScheme == INTERNET_SCHEME_HTTPS) ? WINHTTP_FLAG_SECURE : 0;
		hRequest	= WinHttpOpenRequest(
			   hConnect, L"GET", path, nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
	}

	std::string body;
	std::string errMsg;
	DWORD status = 0;
	if (hRequest
		&& WinHttpSendRequest(
			hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0)
		&& WinHttpReceiveResponse(hRequest, nullptr)) {
		DWORD len = sizeof(status);
		WinHttpQueryHeaders(hRequest,
							WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
							WINHTTP_HEADER_NAME_BY_INDEX,
							&status,
							&len,
							WINHTTP_NO_HEADER_INDEX);
		if (status == 200) {
			DWORD avail = 0;
			while (WinHttpQueryDataAvailable(hRequest, &avail) && avail > 0) {
				std::string chunk(avail, '\0');
				DWORD read = 0;
				if (!WinHttpReadData(hRequest, chunk.data(), avail, &read)) break;
				body.append(chunk.data(), read);
			}
		} else {
			errMsg = "HTTP " + std::to_string(status);
		}
	} else {
		errMsg = "WinHTTP request failed: " + std::to_string(GetLastError());
	}

	if (hRequest) WinHttpCloseHandle(hRequest);
	if (hConnect) WinHttpCloseHandle(hConnect);
	WinHttpCloseHandle(hSession);

	if (!errMsg.empty()) throw DownloadError(url, errMsg);
	return body;
#elif defined(__linux__)
	// curl ships on every distro Koala targets (ubuntu, raspbian). popen avoids
	// pulling libcurl as a build dep for the handful of HTTP calls we make.
	// Single-quote the URL and escape any embedded single quotes so a hostile
	// URL can't break out of the shell argument.
	std::string quoted = "'";
	for (char c: url) {
		if (c == '\'') quoted += "'\\''";
		else quoted += c;
	}
	quoted += "'";
	std::string cmd = "curl -sSfL --max-time 10 " + quoted;

	FILE *pipe = popen(cmd.c_str(), "r");
	if (!pipe) throw DownloadError(url, "popen failed");

	std::string body;
	std::array<char, 4096> buf {};
	while (size_t n = fread(buf.data(), 1, buf.size(), pipe)) {
		body.append(buf.data(), n);
	}
	int rc = pclose(pipe);
	if (rc != 0) {
		throw DownloadError(url, "curl exited " + std::to_string(rc));
	}
	return body;
#else
	Log::e() << "No implementation of downloadUrl yet";
	return "";
#endif
}

std::string postToUrl(const std::string url, const std::vector<std::pair<std::string, std::string>> &params) {
#ifdef __APPLE__
	NSMutableURLRequest *urlRequest = [[NSMutableURLRequest alloc]
		initWithURL:[NSURL URLWithString:[NSString stringWithUTF8String:url.c_str()]]];

	std::string paramStr = "";
	for (const auto &p: params) {
		if (!paramStr.empty()) paramStr += "&";
		paramStr += urlencode(p.first) + "=" + urlencode(p.second);
	}

	NSString *paramNSStr = [NSString stringWithUTF8String:paramStr.c_str()];

	//create the Method "GET" or "POST"
	[urlRequest setHTTPMethod:@"POST"];

	//Convert the String to Data
	NSData *data = [paramNSStr dataUsingEncoding:NSUTF8StringEncoding];

	//Apply the data to the body
	[urlRequest setHTTPBody:data];

	NSURLSession *session = [NSURLSession sharedSession];

	__block bool waitForMe = true;
	__block std::string responseValue;
	__block int responseCode = 0;
	__block int success		 = false;
	NSURLSessionDataTask *dataTask =
		[session dataTaskWithRequest:urlRequest
				   completionHandler:^(NSData *data, NSURLResponse *response, NSError *error) {
					 NSHTTPURLResponse *httpResponse = (NSHTTPURLResponse *) response;

					 responseCode = (int) httpResponse.statusCode;

					 if (httpResponse.statusCode == 200) {
						 NSString *str = [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];
						 if (str == nil) {
							 success	   = false;
							 responseValue = "Couldn't convert NSData to string in postToUrl";
						 } else {
							 success	   = true;
							 responseValue = [str UTF8String];
						 }

					 } else {
						 success = false;
					 }

					 waitForMe = false;
				   }];

	[dataTask resume];

	while (waitForMe) {
		sleepMicros(1);
	}

	if (!success) {
		throw DownloadError(url, responseValue);
	}
	return responseValue;
#else
	// not implemented yet
	Log::e() << "ERROR: no implementation of postToUrl() on this platform";
	return "";
#endif
}
#if MZGL_ANDROID
#	include "androidUtil.h"
#endif
void pingUrl(const std::string &_url) {
#ifdef __APPLE__
	std::string url = _url;
	dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
	  @autoreleasepool {
		  NSURL *nsUrl = [NSURL URLWithString:[NSString stringWithUTF8String:url.c_str()]];
		  if (!nsUrl) return;

		  NSURLSession *session		 = [NSURLSession sharedSession];
		  NSURLSessionDataTask *task = [session dataTaskWithURL:nsUrl];
		  [task resume];
	  }
	});
#elif MZGL_ANDROID
	callJNI("pingUrl", _url);
#endif
}
std::string urlencode(const std::string &value) {
	static auto hex_digt = "0123456789ABCDEF";

	std::string result;
	result.reserve(value.size() << 1);

	for (auto ch: value) {
		if ((ch >= '0' && ch <= '9') || (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') || ch == '-'
			|| ch == '_' || ch == '!' || ch == '\'' || ch == '(' || ch == ')' || ch == '*' || ch == '~'
			|| ch == '.') /*  !'()*-._~   */ {
			result.push_back(ch);
		} else {
			result += std::string("%") + hex_digt[static_cast<unsigned char>(ch) >> 4]
					  + hex_digt[static_cast<unsigned char>(ch) & 15];
		}
	}

	return result;
}

std::string urldecode(const std::string &value) {
	std::string result;
	result.reserve(value.size());

	for (std::size_t i = 0; i < value.size(); ++i) {
		auto ch = value[i];

		if (ch == '%' && (i + 2) < value.size()) {
			auto hex = value.substr(i + 1, 2);
			auto dec = static_cast<char>(std::strtol(hex.c_str(), nullptr, 16));
			result.push_back(dec);
			i += 2;
		} else if (ch == '+') {
			result.push_back(' ');
		} else {
			result.push_back(ch);
		}
	}

	return result;
}

static std::string encodeURIComponent(const std::string &input) {
	std::ostringstream encoded;
	for (unsigned char c: input) {
		if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~' || c == '/') {
			encoded << c;
		} else {
			encoded << '%' << std::uppercase << std::hex << static_cast<int>(c);
		}
	}
	return encoded.str();
}
#include "filesystem.h"

std::string pathToFileUrl(const std::string &path) {
	std::string normalizedPath = path;
	try {
		fs::path absPath		   = fs::absolute(path);
		std::string normalizedPath = absPath.generic_string(); // Converts to forward slashes
	} catch (const std::exception &e) {
		Log::e() << "Error normalizing path: " << e.what();
	}
	// Handle Windows drive letters
#ifdef _WIN32
	if (normalizedPath.size() > 1 && normalizedPath[1] == ':') {
		normalizedPath.insert(0, "/"); // Prepend a slash before drive letter
	}
#endif

	return "file://" + encodeURIComponent(normalizedPath);
}