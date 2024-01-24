//
//  RoundedRect.hpp
//  linerender
//
//  Created by Marek Bereza on 28/02/2018.
//  Copyright © 2018 Marek Bereza. All rights reserved.
//

#pragma once

#include <mzgl/geom/Rectf.h>
#include <mzgl/gl/Vbo.h>
#include <mzgl/gl/Graphics.h>

void makeRoundedRectVbo(VboRef m, const Rectf &r, float radius, bool solid = true, float strokeWeight = 1);

void getRoundedRectVerts(const Rectf &r, float radius, std::vector<glm::vec2> &outVerts);

// not used by the RoundedRect class, as it doesn't use a cache, slower, more accurate
void getPerfectRoundedRectVerts(const Rectf &r, float radius, std::vector<glm::vec2> &outVerts);
/**
 * This class draws rounded rects, caching the vertices
 * in a VBO if the size doesn't change.
 */
class RoundedRect {
public:
	VboRef mesh;

	void draw(Graphics &g, const Rectf &r, float radius);

	// resets cache and forces redraw
	void touch();

private:
	float oldStrokeWeight = 1;
	Rectf oldRect;
	bool oldSolid	= false;
	float oldRadius = -1;
};
