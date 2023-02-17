//
//  ScopedMask.h
//  mzgl
//
//  Created by Marek Bereza on 17/02/2023.
//  Copyright © 2023 Marek Bereza. All rights reserved.
//

#pragma once

#include "Graphics.h"

struct ScopedMask {
	

	// you can initialize with empty constructor
	// so there's no mask actually happening
	// this is handy when you want conditinal masks
	ScopedMask() {
		masking = false;
	}
	ScopedMask(Graphics &g, const Rectf &r) {
		startMask(g, r);
	}

	void startMask(Graphics &g, const Rectf &r);

	void stopMask();

	virtual ~ScopedMask() {
		stopMask();
	}
private:
	bool scissorWasEnabled = false;
	Rectf rect;
	Graphics *graphics;
	bool masking = false;
};
