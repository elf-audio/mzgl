//
//  Triangulator.h
//  roundedtexrect
//
//  Created by Marek Bereza on 01/03/2018.
//  Copyright © 2018 Marek Bereza. All rights reserved.
//

#pragma once


#include "MPE_fastpoly2tri.h"

#include "Graphics.h"
class Triangulator {
public:
	// first member of verts is the shape, next ones are the holes
	int triangulate(std::vector<std::vector<glm::vec2>> &verts, std::vector<glm::vec2> &outVerts, std::vector<unsigned int> &indices);
};
