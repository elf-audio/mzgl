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
	
	void update() {
		if(enabled) {
			amt += 0.05f;
			if(amt>1) amt = 1.f;
		} else {
			amt -= 0.05f;
			if(amt<0.f) amt = 0.f;
		}
	}
	// returns true if amt is more than zero or is going to be
	bool isOn() const {
		return enabled || amt > 0.01f;
	}
	
private:
	
	float amt = 0.f;
	bool enabled = false;
};
