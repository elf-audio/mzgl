//
//  ShapeLUT.h
//  mzgl
//
//  Immutable, lazily-built lookup tables for tessellating circles and
//  rounded-rect corners without doing trig on every draw call.
//
//  Both accessors return a const ref into a function-local static built once
//  on first use. C++11 guarantees that init is thread-safe and happens exactly
//  once, so this is safe across multiple plugin instances - the data is
//  read-only, never mutated, so there is nothing to race on or overwrite.
//
//  Resolution is bucketed: you ask for "at least N segments" and get the
//  smallest precomputed table that satisfies it (capped at the largest
//  bucket). Small shapes stay cheap, large ones stay smooth.
//

#pragma once

#include <vector>
#include <glm/glm.hpp>

// Full unit circle, points are {cos(theta), sin(theta)}.
// Returns a table with >= minSegments points (no duplicated closing point).
const std::vector<glm::vec2> &unitCircleLUT(int minSegments);

// One rounded-rect corner: a quarter arc in (1 + cos(phi), 1 + sin(phi)) form,
// phi sweeping [PI, 3PI/2]. Matches the old createRoundedRectCache layout so
// roundedRectVerts() can consume it unchanged.
// Returns a table with >= minSteps points.
const std::vector<glm::vec2> &roundedRectCornerLUT(int minSteps);

// How many points to tessellate one rounded-rect corner of the given radius,
// targeting ~pixelsPerStep pixels per segment. Trig-free (small-angle form of
// the old 2*PI / (asin(pixelsPerStep/2/radius)/2)) - the result is bucketed by
// roundedRectCornerLUT anyway, so the approximation is harmless.
int roundedRectCornerSteps(float radius, float pixelsPerStep);
