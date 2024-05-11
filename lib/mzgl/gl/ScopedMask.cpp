//  Created by Marek Bereza on 17/02/2023.
#include "ScopedMask.h"

void ScopedMask::startMask(Graphics &g, const Rectf &r) {
	masking	 = true;
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
