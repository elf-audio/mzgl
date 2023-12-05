//
//  ScopedUrl.cpp
//  mzgl
//
//  Created by Marek Bereza on 01/12/2023.
//  Copyright © 2023 Marek Bereza. All rights reserved.
//

#include "ScopedUrl.h"
#include "filesystem.h"
#include "log.h"

ScopedUrl::~ScopedUrl() {
	destructEarly();
}


void ScopedUrl::destructEarly() {
	if(callback) callback();
	callback = nullptr;
	if(shouldTryToDelete) {
		shouldTryToDelete = false;
		try {
			if(fs::exists(url) && fs::is_regular_file(url)) {
				fs::remove(url);
			}
		} catch (fs::filesystem_error &e) {
			Log::e() << "Couldn't remove file: " << e.what();
		}
	}
}
