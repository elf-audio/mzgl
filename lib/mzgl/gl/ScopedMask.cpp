//
//  ScopedMask.cpp
//  mzgl
//
//  Created by Marek Bereza on 17/02/2023.
//  Copyright © 2023 Marek Bereza. All rights reserved.
//

#include "ScopedMask.h"

void ScopedMask::startMask(Graphics &g, const Rectf &r) {
	masking = true;

	graphics = &g;
	if (graphics->isMaskOn()) {
		scissorWasEnabled = true;

		rect			  = graphics->getMaskRect();
	}
	graphics->maskOn(r);
}

void ScopedMask::stopMask() {
	if (!masking) return;

	if (scissorWasEnabled) {
		graphics->maskOn(rect);
	} else {
		graphics->maskOff();
	}

}
