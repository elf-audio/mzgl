//
//  mzAssert.h
//  mzgl
//
//  Created by Marek Bereza on 04/08/2022.
//  Copyright © 2022 Marek Bereza. All rights reserved.
//

#pragma once

#include <cassert>
#if defined(DEBUG) && !defined(AUTO_TEST)
#	include "log.h"
#	define mzAssertImpl(COND, MESSAGE)                                                                           \
		do {                                                                                                      \
			if (mzAssertEnabled()) {                                                                              \
				bool a = (COND);                                                                                  \
				if (!a) {                                                                                         \
					std::string msgStr = MESSAGE; /* Convert MESSAGE to std::string */                            \
                                                                                                                  \
					std::string errorMsg = "ASSERTION FAILED IN " + std::string(__FILE__) + " at line "           \
										   + std::to_string(__LINE__) + (msgStr.empty() ? "" : (": " + msgStr));  \
					Log::e() << errorMsg;                                                                         \
				}                                                                                                 \
				assert(a);                                                                                        \
			}                                                                                                     \
		} while (0)

#else
#	define mzAssertImpl(A, ...) {};

#endif

#define mzAssert(A, ...)  mzAssertImpl((A), #__VA_ARGS__)
#define mzAssertFail(...) mzAssertImpl(false, #__VA_ARGS__)

void mzEnableAssert(bool enabled);
bool mzAssertEnabled();

class MZScopedAssertDisable {
public:
	MZScopedAssertDisable() { mzEnableAssert(false); }
	~MZScopedAssertDisable() { mzEnableAssert(true); }
};
