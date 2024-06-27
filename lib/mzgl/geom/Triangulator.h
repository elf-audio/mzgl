//
//  Triangulator.h
//  roundedtexrect
//
//  Created by Marek Bereza on 01/03/2018.
//  Copyright © 2018 Marek Bereza. All rights reserved.
//

#pragma once

#include <mzgl/gl/Graphics.h>

class Triangulator {
public:
	// first member of verts is the shape, next ones are the holes
	// returns number of verts created
	int triangulate(std::vector<std::vector<glm::vec2>> &verts,
					std::vector<glm::vec2> &outVerts,
					std::vector<unsigned int> &indices);
};
