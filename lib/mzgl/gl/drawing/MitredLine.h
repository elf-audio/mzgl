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
	enum class EndCap {
		None,
		Square,
		Round,
	};

	enum class OpenOrClosed {
		Open,
		Closed,
	};
	// if outside is false, we draw on the line
	// if outside is true, then we draw on the outside of the line
	bool outside = false;

	bool mitreLimit = false;

	void getStripCoord(
		const glm::vec2 &before, const glm::vec2 &curr, const glm::vec2 &after, glm::vec2 &a, glm::vec2 &b);

	int getVerts(const std::vector<glm::vec2> &p,
				 std::vector<glm::vec2> &outVerts,
				 std::vector<unsigned int> &indices,
				 OpenOrClosed openClosed = OpenOrClosed::Open,
				 bool bevelled			 = false,
				 EndCap endCap			 = EndCap::None);
	int getVertsBevelled(const std::vector<glm::vec2> &p,
						 std::vector<glm::vec2> &outVerts,
						 std::vector<unsigned int> &indices,
						 OpenOrClosed openClosed = OpenOrClosed::Open,
						 EndCap endCap			 = EndCap::None);

private:
	void doRoundCap(glm::vec2 p0,
					glm::vec2 p1,
					std::vector<glm::vec2> &outVerts,
					std::vector<unsigned int> &indices);

	LineEquation top1;
	LineEquation top2;
	LineEquation bottom1;
	LineEquation bottom2;
	inline glm::vec2 getNormal(glm::vec2 a) {
		a = glm::normalize(a);
		a = glm::vec2(-a.y, a.x);
		a *= thickness * 0.5;
		return a;
	}
};
