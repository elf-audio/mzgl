//
//  mzAssert.cpp
//  mzgl
//
//  Created by Marek Bereza on 04/08/2022.
//  Copyright © 2022 Marek Bereza. All rights reserved.
//

#include <mzgl/util/mzAssert.h>

bool assertsEnabled = true;

void mzEnableAssert(bool enabled) {
	assertsEnabled = enabled;
}

bool mzAssertEnabled() {
	return assertsEnabled;
}
