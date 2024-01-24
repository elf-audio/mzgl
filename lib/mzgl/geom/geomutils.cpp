//
//  geomutils.cpp
//  mzgl
//
//  Created by Marek Bereza on 28/09/2022.
//  Copyright © 2022 Marek Bereza. All rights reserved.
//

#include <mzgl/geom/geomutils.h>
#include <algorithm>

static inline float length_squared(const vec2 &v, const vec2 &w) noexcept {
	const float x = v.x - w.x;
	const float y = v.y - w.y;
	return x * x + y * y;
}

//https://stackoverflow.com/questions/849211/shortest-distance-between-a-point-and-a-line-segment
float pointLineSegmentMinDist(vec2 v, vec2 w, vec2 p) {
	// Return minimum distance between line segment vw and point p
	const float l2 = length_squared(v, w); // i.e. |w-v|^2 -  avoid a sqrt
	if (l2 == 0.f) return distance(p, v); // v == w case
	// Consider the line extending the segment, parameterized as v + t (w - v).
	// We find projection of point p onto the line.
	// It falls where t = [(p-v) . (w-v)] / |w-v|^2
	// We clamp t from [0,1] to handle points outside the segment vw.
	const float t		  = std::max(0.f, std::min(1.f, dot(p - v, w - v) / l2));
	const vec2 projection = v + t * (w - v); // Projection falls on the segment
	return distance(p, projection);
}

static float loopSign(vec2 p1, vec2 p2, vec2 p3) {
	return (p1.x - p3.x) * (p2.y - p3.y) - (p2.x - p3.x) * (p1.y - p3.y);
}

bool pointIsInsideTriangle(vec2 pt, vec2 v1, vec2 v2, vec2 v3) {
	bool b1, b2, b3;

	b1 = loopSign(pt, v1, v2) < 0.f;
	b2 = loopSign(pt, v2, v3) < 0.f;
	b3 = loopSign(pt, v3, v1) < 0.f;

	return ((b1 == b2) && (b2 == b3));
}

float whichSideOfLineIsPoint(const vec2 &a, const vec2 &b, const vec2 &p) {
	return ((a.y - b.y) * p.x + (b.x - a.x) * p.y + (a.x * b.y - b.x * a.y));
}

float pointLineMinDist(const vec2 &a, const vec2 &b, const vec2 &p) {
	// from http://www.softsurfer.com/Archive/algorithm_0102/algorithm_0102.htm#Distance to 2-Point Line
	return abs(((a.y - b.y) * p.x + (b.x - a.x) * p.y + (a.x * b.y - b.x * a.y))
			   / sqrt((b.x - a.x) * (b.x - a.x) + (b.y - a.y) * (b.y - a.y)));
}
