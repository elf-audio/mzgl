//
//  ScopedMask.cpp
//  mzgl
//
//  Created by Marek Bereza on 17/02/2023.
//  Copyright © 2023 Marek Bereza. All rights reserved.
//

#include "ScopedMask.h"
//#include "mzOpenGL.h"

void ScopedMask::startMask(Graphics &g, const Rectf &r) {
	masking = true;
//	if(glIsEnabled(GL_SCISSOR_TEST)) {
//		scissorWasEnabled = true;
//		glGetFloatv(GL_SCISSOR_BOX, vals);
//	}
//	glEnable(GL_SCISSOR_TEST);
//
//	// glScissor works with pixel coordinates
//	// so make sure that parameters are not
//	// in different coord-space. If thats the case,
//	// some kind of translation must take place here
//	// see: wrapMaskForScissors
//	glScissor(r.x, g.height-(r.bottom()), r.width, r.height);
//
	graphics = &g;
	if(graphics->isMaskOn()) {
		scissorWasEnabled = true;
		rect = graphics->getMaskRect(); //glGetFloatv(GL_SCISSOR_BOX, vals);
	}
	graphics->maskOn(r);
}

void ScopedMask::stopMask() {
	if(!masking) return;
	
	if(scissorWasEnabled) {
		graphics->maskOn(rect);
	} else {
		graphics->maskOff();
	}
//	if(scissorWasEnabled) {
//		glScissor(vals[0], vals[1], vals[2], vals[3]);
//	} else {
//		glDisable(GL_SCISSOR_TEST);
//	}
}
