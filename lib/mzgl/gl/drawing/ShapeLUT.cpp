//
//  ShapeLUT.cpp
//  mzgl
//

#include "ShapeLUT.h"
#include "maths.h"
#include <cmath>

namespace {

constexpr float kPi		= static_cast<float>(M_PI);
constexpr float kTwoPi	= static_cast<float>(2.0 * M_PI);

// A set of precomputed tables at increasing segment counts plus a picker
// that returns the smallest one with >= the requested count.
struct LUTSet {
	std::vector<int> counts;
	std::vector<std::vector<glm::vec2>> tables;

	const std::vector<glm::vec2> &pick(int want) const {
		for (size_t i = 0; i < counts.size(); i++) {
			if (counts[i] >= want) return tables[i];
		}
		return tables.back();
	}
};

} // namespace

const std::vector<glm::vec2> &unitCircleLUT(int minSegments) {
	static const LUTSet lut = [] {
		LUTSet s;
		for (int n: {6, 8, 10, 12, 16, 20, 24, 32, 40, 48, 64, 80, 100}) {
			std::vector<glm::vec2> pts(n);
			for (int i = 0; i < n; i++) {
				float th = kTwoPi * (float) i / (float) n;
				pts[i]	 = {std::cos(th), std::sin(th)};
			}
			s.counts.push_back(n);
			s.tables.push_back(std::move(pts));
		}
		return s;
	}();
	return lut.pick(minSegments);
}

const std::vector<glm::vec2> &roundedRectCornerLUT(int minSteps) {
	static const LUTSet lut = [] {
		LUTSet s;
		for (int n: {2, 3, 4, 6, 8, 12, 16, 24, 32, 48, 64, 96, 128}) {
			std::vector<glm::vec2> pts(n);
			for (int i = 0; i < n; i++) {
				float phi = mapf((float) i, 0.f, (float) n, kPi, kPi + kPi / 2.f);
				pts[i]	  = {1.f + std::cos(phi), 1.f + std::sin(phi)};
			}
			s.counts.push_back(n);
			s.tables.push_back(std::move(pts));
		}
		return s;
	}();
	return lut.pick(minSteps);
}
