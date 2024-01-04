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
		Bookmark(NSData *bookmarkData, NSURL *url)
			: bookmarkData(bookmarkData)
			, url(url) {
			path = [[url path] UTF8String];
		}

		virtual ~Bookmark() {
			Log::d() << "STOPPING ACCESS FOR '" << path << "'";
#if !defined(DEBUG) || TARGET_OS_IOS

			[url stopAccessingSecurityScopedResource];
#else
#	pragma warning For some reason, this crashes in CLion builds but not Xcode builds
			// it happens on destruction of the App, and url becomes invalid memory somehow.
			// Maybe we should use [NSApp stop] rather than [NSApp terminate] and let
			// control flow to end of main func nicely (see comment in mainMac.mm)
			Log::d() << "Not actually stopping access for " << path << " because of a bug";
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

		NSError *err;

		NSURLBookmarkCreationOptions opts =
#if TARGET_OS_IOS
			NSURLBookmarkCreationSuitableForBookmarkFile;
#else
			NSURLBookmarkCreationWithSecurityScope | NSURLBookmarkCreationSecurityScopeAllowOnlyReadAccess;
#endif
		NSData *bookmarkData =
			[url bookmarkDataWithOptions:opts includingResourceValuesForKeys:nil relativeToURL:nil error:&err];

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
		bookies.emplace_back(new Bookmark(bookmarkData, url));
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
			NSDictionary *book =
				@ {@"data" : b->bookmarkData, @"path" : [NSString stringWithUTF8String:b->path.c_str()]};
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
				NSError *err;

				NSURLBookmarkResolutionOptions opts =
#if TARGET_OS_IOS
					0;
#else
					NSURLBookmarkResolutionWithSecurityScope;
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

				if (![url startAccessingSecurityScopedResource]) {
					NSLog(@"Failed to startAccessingSecurityScopedResource");
					continue;
				}
				printf("GOT SCOPED ACCESS FOR '%s'\n", [[url path] UTF8String]);
				bookies.emplace_back(new Bookmark(b[@"data"], url));
			}
		}
	}
};
