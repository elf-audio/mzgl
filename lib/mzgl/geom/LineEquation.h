//
//  Rectf.hpp
//  samploid
//
//  Created by Marek Bereza on 16/02/2018.
//  Copyright © 2018 Marek Bereza. All rights reserved.
//

#pragma once
#include <glm/glm.hpp>

#include <vector>

class LineEquation {
public:
	
	float m;
	float c;
	bool vertical = false;
	
	void setFrom2Points(glm::vec2 a, glm::vec2 b) {
		if(a.x == b.x) {
			vertical = true;
			m = std::numeric_limits<float>::infinity();
			c = a.x;
		} else {
			vertical = false;
			glm::vec2 g = b - a;
			m = g.y / g.x;
		
			// y = mx + c
			// so c = y - mx
			c = a.y - m*a.x;
		}
	}
	
	float distanceToPoint(const glm::vec2 &p) {
		glm::vec2 a(1, getY(1));
		glm::vec2 b(2, getY(2));
		// from http://www.softsurfer.com/Archive/algorithm_0102/algorithm_0102.htm#Distance to 2-Point Line
		return std::abs(((a.y - b.y)*p.x + (b.x - a.x)*p.y + (a.x*b.y - b.x*a.y))/
				   sqrt((b.x-a.x)*(b.x-a.x) + (b.y - a.y)*(b.y - a.y))
				   );
	}
	
	/**
	 * Returns the index of the closest point in the array.
	 */
	int closestPoint(const std::vector<glm::vec2> &p) {
		glm::vec2 a(1, getY(1));
		glm::vec2 b(2, getY(2));
		
		
		// from http://www.softsurfer.com/Archive/algorithm_0102/algorithm_0102.htm#Distance to 2-Point Line
		float amby = a.y - b.y;
		float bmax = b.x - a.x;
		
		float axbymbxay = (a.x*b.y - b.x*a.y);
		float minDist = FLT_MAX;
		int minI = 0;
		for(int i = 0; i < p.size(); i++) {
			
			float dist = std::abs((float)(amby*p[i].x + bmax*p[i].y + axbymbxay));
			if(dist<minDist) {
				minDist = dist;
				minI = i;
			}
		}
		
		return minI;
	}
	
	glm::vec2 getIntersection(const LineEquation &l2) {
		
		if(l2.m==m) {
			return glm::vec2(NAN,NAN);
		} else if(vertical) {
			return glm::vec2(c, l2.getY(c));
		} else if(l2.vertical) {
			return glm::vec2(l2.c, getY(l2.c));
		}
		float x = (l2.c - c)/(m - l2.m);
		return glm::vec2(x, getY(x));
	}
	
	float getY(float x) const {
		if(vertical) return NAN;
		return m*x + c;
	}
	
	float getX(float y) const {
		if(vertical) return c;
		return (y-c)/m;
	}
};
