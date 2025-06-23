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
//
#	define mzAssertNoMessage(A)                                                                                  \
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

#	define mzAssertWithMessage(A, ...)                                                                           \
		do {                                                                                                      \
			if (mzAssertEnabled()) {                                                                              \
				bool a = (A);                                                                                     \
				if (!a) {                                                                                         \
					Log::e() << "ASSERTION FAILED IN " << __FILE__ << " at line " << __LINE__ << ": "             \
							 << __VA_ARGS__;                                                                      \
				}                                                                                                 \
				assert(a);                                                                                        \
			}                                                                                                     \
		} while (0)
#	define selectMzAssert(_1, _2, NAME, ...) NAME
#	define mzAssert(...)					  selectMzAssert(__VA_ARGS__, mzAssertWithMessage, mzAssertNoMessage)(__VA_ARGS__)

#else
#	define mzAssert(...) {};
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
