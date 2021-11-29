
//
//  AlphaButton.h
//
//
//

#pragma once

#include "SimpleButton.h"

class AlphaButton: public SimpleButton {
public:
	
	
	AlphaButton(string name, string img): SimpleButton(name, img, "") {}
	void draw() {
		glm::vec4 c = color;
		if(down) {
			c.a *= 0.5;
		}
		ofSetColor(c);
		upImg.draw(*this);
		
	}
};
