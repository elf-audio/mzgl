//
//  netUtil.cpp
//  mzgl
//
//  Created by Marek Bereza on 16/05/2023.
//  Copyright © 2023 Marek Bereza. All rights reserved.
//

#include "netUtil.h"
#ifdef __APPLE__
#	include <Foundation/Foundation.h>
#endif

std::string downloadUrl(std::string url) {
#ifdef __APPLE__
	// the URL to save
	NSURL *urlNS = [NSURL URLWithString:[NSString stringWithUTF8String: url.c_str()]];
	
	NSError* error = nil;
	// turn it into a request and use NSData to load its content
//	NSURLRequest *request = [NSURLRequest requestWithURL:urlNS];
//	NSData *data = [NSURLConnection sendSynchronousRequest:request returningResponse:nil error:nil];
	NSData* data = [NSData dataWithContentsOfURL:urlNS options:NSDataReadingUncached error:&error];

	if (error != nil) {
		NSLog(@"Error downloading data: %@", error);
		std::string errorMessage = [NSString stringWithFormat:@"Error downloading data: %@", error].UTF8String;
		throw DownloadError(url, errorMessage);
   }
	
	NSString *str = [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];
	if(str==nil) {
		throw DownloadError(url, "Couldn't convert NSData to string in downloadUrl");
	}
	return [str UTF8String];
#else
	Log::e() << "No implementation of downloadUrl yet";
	return "";
#endif
}
