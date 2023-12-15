//
//  geomutils.h
//  mzgl
//
//  Created by Marek Bereza on 28/09/2022.
//  Copyright © 2022 Marek Bereza. All rights reserved.
//

#pragma once
#include <glm/glm.hpp>

using namespace glm;

/**
 * Shortest distance between point p and line segment [v,w]
 */
float pointLineSegmentMinDist(vec2 v, vec2 w, vec2 p);

/**
 * Shortest distance between line that a and b sit on, and the point p
 */
float pointLineMinDist(const vec2 &a, const vec2 &b, const vec2 &p);

bool pointIsInsideTriangle(vec2 pt, vec2 v1, vec2 v2, vec2 v3);

/**
 * Gives a value which will change sign if point moves to
 * different side of the line.
 */
float whichSideOfLineIsPoint(const vec2 &a, const vec2 &b, const vec2 &p);
