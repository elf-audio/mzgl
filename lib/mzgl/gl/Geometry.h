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
	std::vector<vec2> verts;
	std::vector<vec4> cols;
	std::vector<uint32_t> indices;
	std::vector<vec2> texCoords;

	void append(const Geometry &geom) {
		const int startVertIndex = verts.size();
		verts.insert(verts.end(), geom.verts.begin(), geom.verts.end());
		cols.insert(cols.end(), geom.cols.begin(), geom.cols.end());
		texCoords.insert(texCoords.end(), geom.texCoords.begin(), geom.texCoords.end());
		for (unsigned int index: geom.indices) {
			indices.push_back(index + startVertIndex);
		}
	}
	void setColor(glm::vec4 color) {
		for (auto &col: cols)
			col = color;
	}
	void translate(glm::vec2 p) {
		for (auto &vert: verts)
			vert += p;
	}
	void scale(glm::vec2 s) {
		for (auto &vert: verts)
			vert *= s;
	}

	void calculateNormalizedTexCoords() {
		vec2 maxV(FLT_MIN, FLT_MIN);
		vec2 minV(FLT_MAX, FLT_MAX);

		for (auto &vert: verts) {
			if (vert.x > maxV.x) maxV.x = vert.x;
			if (vert.x < minV.x) minV.x = vert.x;

			if (vert.y > maxV.y) maxV.y = vert.y;
			if (vert.y < minV.y) minV.y = vert.y;
		}
		texCoords.resize(verts.size());
		for (int i = 0; i < verts.size(); i++) {
			texCoords[i] =
				vec2((verts[i].x - minV.x) / (maxV.x - minV.x), (verts[i].y - minV.y) / (maxV.y - minV.y));
		}
	}
	[[nodiscard]] bool empty() const { return verts.empty(); }

	void clear() {
		verts.clear();
		cols.clear();
		indices.clear();
		texCoords.clear();
	}
};
