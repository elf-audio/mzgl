//
//  MitredLine.cpp
//  samploid
//
//  Created by Marek Bereza on 16/03/2018.
//  Copyright © 2018 Marek Bereza. All rights reserved.
//

#include "MitredLine.h"
using namespace std;
#include "maths.h"
void MitredLine::getStripCoord(
	const glm::vec2 &before, const glm::vec2 &curr, const glm::vec2 &after, glm::vec2 &a, glm::vec2 &b) {
	glm::vec2 n1 = getNormal(curr - before);
	glm::vec2 n2 = getNormal(after - curr);

	if (outside) {
		top1.setFrom2Points(before, curr);
		top2.setFrom2Points(curr, after);

		bottom1.setFrom2Points(before - n1 * 2.f, curr - n1 * 2.f);
		bottom2.setFrom2Points(curr - n2 * 2.f, after - n2 * 2.f);
	} else {
		top1.setFrom2Points(before - n1, curr - n1);
		top2.setFrom2Points(curr - n2, after - n2);

		bottom1.setFrom2Points(before + n1, curr + n1);
		bottom2.setFrom2Points(curr + n2, after + n2);
	}
	if (abs(top1.m - top2.m) < 0.0001) {
		// parallel, so don't try to find intersection
		if (outside) {
			a = (curr);
			b = (curr - n1 * 2.f);
		} else {
			a = (curr - n1);
			b = (curr + n1);
		}
	} else {
		a = top1.getIntersection(top2);
		b = bottom1.getIntersection(bottom2);
	}
}

static float calculateAngle(glm::vec2 v) {
	return atan2(v.y, v.x);
}

int MitredLine::getVerts(const vector<glm::vec2> &p,
						 vector<glm::vec2> &outVerts,
						 vector<unsigned int> &indices,
						 OpenOrClosed openClosed,
						 bool bevelled,
						 EndCap endCap) {
	int originalVertCount = (int) outVerts.size();

	if (p.size() < 2) return 0;
	vector<glm::vec2> strip;
	if (openClosed == OpenOrClosed::Closed) {
		glm::vec2 a;
		glm::vec2 b;
		getStripCoord(p.back(), p[0], p[1], a, b);
		strip.push_back(a);
		strip.push_back(b);
	} else {
		glm::vec2 n = getNormal(p[1] - p[0]);
		if (endCap == EndCap::Square) {
			glm::vec2 cap(n.y, -n.x);
			strip.push_back(p[0] - n - cap);
			strip.push_back(p[0] + n - cap);
		} else {
			strip.push_back(p[0] - n);
			strip.push_back(p[0] + n);
		}
	}

	if (bevelled) {
		for (int i = 1; i < p.size() - 1; i++) {
			auto n = getNormal(p[i] - p[i - 1]);
			strip.emplace_back(p[i] - n);
			strip.emplace_back(p[i] + n);

			n = getNormal(p[i + 1] - p[i]);
			strip.emplace_back(p[i] - n);
			strip.emplace_back(p[i] + n);
		}
	} else {
		for (int i = 1; i < p.size() - 1; i++) {
			glm::vec2 a;
			glm::vec2 b;
			getStripCoord(p[i - 1], p[i], p[i + 1], a, b);
			if (mitreLimit) {
				if (glm::distance(p[i], a) > thickness * 0.5) {
					auto da = glm::normalize(a - p[i]) * thickness * 0.5f;
					a		= p[i] + da;
				}

				if (glm::distance(p[i], b) > thickness * 0.5) {
					auto db = glm::normalize(b - p[i]) * thickness * 0.5f;
					b		= p[i] + db;
				}
			}
			strip.emplace_back(a);
			strip.emplace_back(b);
		}
	}

	if (openClosed == OpenOrClosed::Closed) {
		glm::vec2 a;
		glm::vec2 b;
		getStripCoord(p[p.size() - 2], p.back(), p[0], a, b);
		strip.push_back(a);
		strip.push_back(b);

		strip.push_back(strip[0]);
		strip.push_back(strip[1]);

	} else {
		glm::vec2 n = getNormal(p[p.size() - 1] - p[p.size() - 2]);
		if (endCap == EndCap::Square) {
			glm::vec2 cap(n.y, -n.x);
			strip.push_back(p[p.size() - 1] - n + cap);
			strip.push_back(p[p.size() - 1] + n + cap);
		} else {
			strip.push_back(p[p.size() - 1] - n);
			strip.push_back(p[p.size() - 1] + n);
		}
	}

	int startIndex = (int) outVerts.size();
	outVerts.insert(outVerts.end(), strip.begin(), strip.end());
	indices.reserve(indices.size() + strip.size() * 3);
	for (int i = 2; i < strip.size(); i += 2) {
		indices.push_back(startIndex + i - 2);
		indices.push_back(startIndex + i);
		indices.push_back(startIndex + i + 1);

		indices.push_back(startIndex + i + 1);
		indices.push_back(startIndex + i - 1);
		indices.push_back(startIndex + i - 2);
	}

	if (endCap == EndCap::Round) {
		doRoundCap(p[0], p[1], outVerts, indices);
		doRoundCap(p[p.size() - 1], p[p.size() - 2], outVerts, indices);
	}

	return (int) outVerts.size() - originalVertCount;
}

void MitredLine::doRoundCap(glm::vec2 p0,
							glm::vec2 p1,
							vector<glm::vec2> &outVerts,
							vector<unsigned int> &indices) {
	// add a circle at the start
	glm::vec2 center = p0;
	outVerts.push_back(center);
	int centerIndex = (int) outVerts.size() - 1;

	const int numSteps		= thickness / 4;
	const float radius		= thickness * 0.5f;
	const float startAngle	= 0;
	const float endAngle	= M_PI;
	const float angleOffset = calculateAngle(p1 - p0) + M_PI / 2.f;
	for (int i = 0; i < numSteps; i++) {
		float angle = mapf(i, 0, numSteps - 1, startAngle, endAngle) + angleOffset;
		outVerts.push_back(center + glm::vec2(radius * cos(angle), radius * sin(angle)));
	}

	for (int i = 0; i < numSteps - 1; i++) {
		indices.push_back(centerIndex);
		indices.push_back(centerIndex + i + 1);
		indices.push_back(centerIndex + i + 2);
	}
}

int MitredLine::getVertsBevelled(const std::vector<glm::vec2> &p,
								 std::vector<glm::vec2> &outVerts,
								 std::vector<unsigned int> &indices,
								 OpenOrClosed openClosed,
								 EndCap endCap) {
	return getVerts(p, outVerts, indices, openClosed, true, endCap);
}
