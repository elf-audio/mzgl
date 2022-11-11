//
//  MitredLineDrawer.h
//  roundedtexrect
//
//  Created by Marek Bereza on 01/03/2018.
//  Copyright © 2018 Marek Bereza. All rights reserved.
//

#pragma once
#include "LineEquation.h"

#include <vector>


class MitredLine {
public:
	
	
	float thickness = 0.5;
	bool squareCap = true;
	
	// if outside is false, we draw on the line
	// if outside is true, then we draw on the outside of the line
	bool outside = false;
	
	
	inline glm::vec2 getNormal(glm::vec2 a) {
		a = glm::normalize(a);
		a = glm::vec2(-a.y, a.x);
		a *= thickness*0.5;
		return a;
	}
	
	bool mitreLimit = false;
	
	LineEquation top1;
	LineEquation top2;
	LineEquation bottom1;
	LineEquation bottom2;
	
	void getStripCoord(const glm::vec2 &before, const glm::vec2 &curr, const glm::vec2 &after, glm::vec2 &a, glm::vec2 &b);
	
	
	int getVerts(const std::vector<glm::vec2> &p, std::vector<glm::vec2> &outVerts, std::vector<unsigned int> &indices, bool close = false, bool bevelled = false);
	int getVertsBevelled(const std::vector<glm::vec2> &p, std::vector<glm::vec2> &outVerts, std::vector<unsigned int> &indices, bool close = false);
};
