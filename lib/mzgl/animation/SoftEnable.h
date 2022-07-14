//
//  SoftEnable.h
//  mzgl
//
//  Created by Marek Bereza on 15/06/2022.
//  Copyright © 2022 Marek Bereza. All rights reserved.
//

#pragma once

class SoftEnable {
public:
	
	bool set(bool en) {
		if(en==enabled) return false;
		enabled = en;
		return true;
	}
	
	float getAmt() const {
		return amt;
	}
	void setDurationInFrames(int f) {
		// default is 1 / 0.05 = 20
		increment = 1.f / (float)f;
	}
	void update() {
		if(enabled) {
			amt += increment;
			if(amt>1) amt = 1.f;
		} else {
			amt -= increment;
			if(amt<0.f) amt = 0.f;
		}
	}
	// returns true if amt is more than zero or is going to be
	bool isOn() const {
		return enabled || amt > 0.01f;
	}
	bool &getBoolPtr() { return enabled; }
private:
	float increment = 0.05f;
	float amt = 0.f;
	bool enabled = false;
};
