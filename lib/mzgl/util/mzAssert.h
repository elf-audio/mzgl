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
				bool a = (A);                                                                                     \
				if (!a) {                                                                                         \
					Log::e() << "ASSERTION FAILED IN " << __FILE__ << " at line " << __LINE__;                    \
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

void mzEnableAssert(bool enabled);
bool mzAssertEnabled();

class MZScopedAssertDisable {
public:
	MZScopedAssertDisable() { mzEnableAssert(false); }

	~MZScopedAssertDisable() { mzEnableAssert(true); }
};
