//
//  mzAssert.h
//  mzgl
//
//  Created by Marek Bereza on 04/08/2022.
//  Copyright © 2022 Marek Bereza. All rights reserved.
//

#pragma once

#include <assert.h>
#if DEBUG==1 && !defined(AUTO_TEST)
#include "log.h"

#define mzAssert(A) if(mzAssertEnabled()) { bool a = (A); if(!a) { Log::e() << "ASSERTION FAILED IN " << __FILE__ << " at line "<< __LINE__;} assert(a);}
#else
#define mzAssert(A) {};
#endif

void mzEnableAssert(bool enabled);
bool mzAssertEnabled();

class MZScopedAssertDisable {
public:
	MZScopedAssertDisable() {
		mzEnableAssert(false);
	}

	~MZScopedAssertDisable() {
		mzEnableAssert(true);
	}
};

