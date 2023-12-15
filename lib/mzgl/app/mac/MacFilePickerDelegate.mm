//
//  MacFilePickerDelegate.cpp
//  mzgl-macOS
//
//  Created by Marek Bereza on 22/06/2022.
//  Copyright © 2022 Marek Bereza. All rights reserved.
//

#include "MacFilePickerDelegate.h"

@implementation FilePickerDelegate {
	NSArray *allowedExts;
	BOOL allowAll;
	BOOL allowFoldersOnly;
}

- (id)init {
	self = [super init];

	if (self != nil) {
		allowAll		 = YES;
		allowFoldersOnly = YES;
		allowedExts		 = @[ @"png", @"tiff", @"jpg", @"gif", @"jpeg" ];
	}
	return self;
}

- (void)enableFoldersOnly:(BOOL)_allowFoldersOnly {
	allowFoldersOnly = _allowFoldersOnly;
}

- (void)setAllowedExtensions:(NSArray *)exts {
	allowedExts = exts;
	allowAll	= NO;
}

- (BOOL)panel:(id)sender shouldEnableURL:(NSURL *)url {
	if (allowAll || allowFoldersOnly) return YES;

	NSString *ext = [url pathExtension];
	if ([ext isEqualToString:@""] || [ext isEqualToString:@"/"] || ext == nil || ext == nil || [ext length] < 1) {
		return YES;
	}

	for (NSString *e in allowedExts) {
		if ([ext caseInsensitiveCompare:e] == NSOrderedSame) {
			return YES;
		}
	}
	return NO;
}

@end
