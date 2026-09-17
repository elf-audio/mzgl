//
//  AppleFileBookmark.h
//  mzgl
//
//  Created by Marek Bereza on 22/06/2022.
//  Copyright © 2022 Marek Bereza. All rights reserved.
//

#pragma once

#include <string>
#include <filesystem>
#include <vector>
#include <memory>
#include <Foundation/Foundation.h>
#include <TargetConditionals.h>

class AppleFileBookmarks {
private:
	struct Bookmark {
		NSData *bookmarkData;
		NSURL *url = nil;
		std::string path;
		bool securityScoped = true;
		Bookmark(NSData *bookmarkData, NSURL *url, bool securityScoped)
			: bookmarkData(bookmarkData)
			, url(url)
			, securityScoped(securityScoped) {
			path = [[url path] UTF8String];
		}

		virtual ~Bookmark() {
			Log::d() << "STOPPING ACCESS FOR '" << path << "'";
#if TARGET_OS_IOS
			[url stopAccessingSecurityScopedResource];
#endif
		}
	};

public:
	AppleFileBookmarks() {
		NSArray *books = [[NSUserDefaults standardUserDefaults] arrayForKey:@"bookmarks"];
		deserializeBookmarks(books);
	}

	bool add(NSURL *url) {
#if TARGET_OS_IOS
		if (![url startAccessingSecurityScopedResource]) {
			printf("bookmarks::add() - Error accessing security scoped resource on %s\n", [[url path] UTF8String]);
			return false;
		}
#endif

		NSError *err = nil;

		NSURLBookmarkCreationOptions opts =
#if TARGET_OS_IOS
			NSURLBookmarkCreationSuitableForBookmarkFile;
#else
			NSURLBookmarkCreationWithSecurityScope | NSURLBookmarkCreationSecurityScopeAllowOnlyReadAccess;
#endif
		NSData *bookmarkData =
			[url bookmarkDataWithOptions:opts includingResourceValuesForKeys:nil relativeToURL:nil error:&err];
		bool securityScoped = true;

#if !TARGET_OS_IOS
		if (err != nil) {
			err			   = nil;
			bookmarkData   = [url bookmarkDataWithOptions:0
						  includingResourceValuesForKeys:nil
										   relativeToURL:nil
												   error:&err];
			securityScoped = false;
		}
#endif

		if (err) {
			NSLog(@"Error creating bookmark %@", err);
			return false;
		}

		std::string keyPath = [[url path] UTF8String];

		// erase any previous version of the bookmark
		// and be sure to stop accessing it.
		for (int i = 0; i < bookies.size(); i++) {
			auto &b = bookies[i];
			if (b->path == keyPath) {
				bookies.erase(bookies.begin() + i);
				break;
			}
		}
		bookies.emplace_back(new Bookmark(bookmarkData, url, securityScoped));
		[[NSUserDefaults standardUserDefaults] setObject:serializeBookmarks() forKey:@"bookmarks"];
		return true;
	}

	void clear() {
		bookies.clear();
		[[NSUserDefaults standardUserDefaults] setObject:serializeBookmarks() forKey:@"bookmarks"];
	}

	std::vector<std::string> getPaths() const {
		std::vector<std::string> paths;
		for (auto &b: bookies) {
			paths.emplace_back(b->path);
		}
		return paths;
	}
	std::string getPath(int i) {
		if (i < 0 || i >= size()) {
			return "";
		}
		return bookies[i]->path.c_str();
	}

	size_t size() const { return bookies.size(); }

	bool erase(int i) {
		if (i < 0 || i >= size()) {
			return false;
		}
		bookies.erase(bookies.begin() + i);
		[[NSUserDefaults standardUserDefaults] setObject:serializeBookmarks() forKey:@"bookmarks"];
		return true;
	}

private:
	std::vector<std::unique_ptr<Bookmark>> bookies;

	NSArray *serializeBookmarks() {
		NSMutableArray *bs = [[NSMutableArray alloc] init];
		for (auto &b: bookies) {
#if TARGET_OS_IOS
			NSDictionary *book =
				@ {@"data" : b->bookmarkData, @"path" : [NSString stringWithUTF8String:b->path.c_str()]};
#else
			NSDictionary *book = @ {
				@"data" : b->bookmarkData,
				@"path" : [NSString stringWithUTF8String:b->path.c_str()],
				@"securityScoped" : @(b->securityScoped)
			};
#endif
			[bs addObject:book];
		}
		return bs;
	}

	void deserializeBookmarks(NSArray *books) {
		bookies.clear();
		if (books == nil) {
			return;
		} else {
			for (id b in books) {
				BOOL isStale = NO;
				NSError *err = nil;

#if TARGET_OS_IOS
				const bool securityScoped					= true;
				const NSURLBookmarkResolutionOptions opts	= 0;
#else
				NSNumber *isScoped							= b[@"securityScoped"];
				const bool securityScoped					= isScoped == nil || [isScoped boolValue];
				const NSURLBookmarkResolutionOptions opts	= securityScoped
																  ? NSURLBookmarkResolutionWithSecurityScope
																  : 0;
#endif

				NSURL *url = [NSURL URLByResolvingBookmarkData:b[@"data"]
													   options:opts
												 relativeToURL:nil
										   bookmarkDataIsStale:&isStale
														 error:&err];

				printf("Trying to deserialize bookmark '%s'\n", [[url path] UTF8String]);
				if (!url) {
					NSLog(@"Got error trying to resolve bookmark: %@", err);
					continue;
				}
				if (isStale) {
					NSLog(@"file is stale!");
					continue;
				}

				if (securityScoped && ![url startAccessingSecurityScopedResource]) {
					NSLog(@"Failed to startAccessingSecurityScopedResource");
					continue;
				}
				printf("GOT SCOPED ACCESS FOR '%s'\n", [[url path] UTF8String]);
				bookies.emplace_back(new Bookmark(b[@"data"], url, securityScoped));
			}
		}
	}
};
