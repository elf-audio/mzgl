//
//  Geometry.h
//  mzgl
//
//  Created by Marek Bereza on 13/07/2021.
//  Copyright © 2021 Marek Bereza. All rights reserved.
//

#pragma once

#include <vector>

struct Geometry {
	// TODO: This class should underpin Vbo and be in its own file
	std::vector<vec2> verts;
	std::vector<vec4> cols;
	std::vector<uint32_t> indices;
	std::vector<vec2> texCoords;
	
	void setColor(glm::vec4 color) {
		for(size_t i = 0; i < cols.size(); i++) cols[i] = color;
	}
	void translate(glm::vec2 p) {
		for(size_t i = 0; i < verts.size(); i++) verts[i] += p;
	}
	void scale(glm::vec2 s) {
		for(size_t i = 0; i < verts.size(); i++) verts[i] *= s;
	}
	
	void calculateNormalizedTexCoords() {
		
		vec2 maxV(FLT_MIN, FLT_MIN);
		vec2 minV(FLT_MAX, FLT_MAX);
		
		for(int i = 0; i < verts.size(); i++) {
			if(verts[i].x > maxV.x) maxV.x = verts[i].x;
			if(verts[i].x < minV.x) minV.x = verts[i].x;
			
			if(verts[i].y > maxV.y) maxV.y = verts[i].y;
			if(verts[i].y < minV.y) minV.y = verts[i].y;
		}
		texCoords.resize(verts.size());
		for(int i = 0; i < verts.size(); i++) {
			texCoords[i] = vec2(
				(verts[i].x - minV.x) / (maxV.x - minV.x),
				(verts[i].y - minV.y) / (maxV.y - minV.y)
			);
		}
	}

	
	void clear() {
		verts.clear();
		cols.clear();
		indices.clear();
		texCoords.clear();
	}
};
