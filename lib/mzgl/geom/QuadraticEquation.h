//
//  QuadraticEquation.h
//  parabola
//
//  Created by Marek Bereza on 13/03/2019.
//  Copyright © 2019 Marek Bereza. All rights reserved.
//

#pragma once
#include <glm/glm.hpp>
class QuadraticEquation {
public:
	// y = ax^2 + bx + c
	float a = 1;
	float b = 0;
	float c = 0;

	void setFrom3Points(glm::vec2 p1, glm::vec2 p2, glm::vec2 p3) {
		float A1	= -(p1.x * p1.x) + p2.x * p2.x;
		float B1	= -p1.x + p2.x;
		float D1	= -p1.y + p2.y;
		float A2	= -(p2.x * p2.x) + p3.x * p3.x;
		float B2	= -p2.x + p3.x;
		float D2	= -p2.y + p3.y;
		float Bmult = -(B2 / B1);
		float A3	= Bmult * A1 + A2;
		float D3	= Bmult * D1 + D2;

		a = D3 / A3;
		b = (D1 - A1 * a) / B1;
		c = p1.y - a * (p1.x * p1.x) - b * p1.x;
	}

	float getY(float x) { return a * x * x + b * x + c; }
	float getExtremeX() { return -b / (2.f * a); }
	float getExtremeY() { return getY(getExtremeX()); }
	glm::vec2 getExtreme() {
		float ex = getExtremeX();
		return {ex, getY(ex)};
	}
};
