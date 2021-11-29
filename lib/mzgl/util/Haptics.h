//
//  Haptics.h
//  mzgl
//
//  Created by Marek Bereza on 16/06/2021.
//  Copyright © 2021 Marek Bereza. All rights reserved.
//

#pragma once
#include <memory>
class HapticsImpl;

class Haptics {
public:
	Haptics();
	
	virtual ~Haptics();
	void lightTap();
	std::shared_ptr<HapticsImpl> impl;
};
