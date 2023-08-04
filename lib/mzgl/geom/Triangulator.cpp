//
//  Triangulator.h
//  roundedtexrect
//
//  Created by Marek Bereza on 01/03/2018.
//  Copyright © 2018 Marek Bereza. All rights reserved.
//

#include "Triangulator.h"
#define MPE_POLY2TRI_IMPLEMENTATION
#include "MPE_fastpoly2tri.h"
#include "log.h"

using namespace std;

// first member of verts is the shape, next ones are the holes
int Triangulator::triangulate(vector<vector<glm::vec2>> &verts, vector<glm::vec2> &outVerts, vector<unsigned int> &indices) {
	// check for adjacent duplicates
	for (int k = 0; k < verts.size(); k++) {
		for (int i = 0; i < verts[k].size(); i++) {
			for (int j = i + 1; j < verts[k].size(); j++) {
				if (distance(verts[k][i], verts[k][j]) < 0.0001) { // this should be FLT_EPSILON MAYBE? or at least match the triangulator
					//Log::e() << "found duplicate";
					verts[k].erase(verts[k].begin() + j);
					j--;
				} else {
					break;
				}
			}
		}
		while (verts[k].size() > 2 && verts[k].back() == verts[k][0]) {
			verts[k].pop_back();
		}
	}
	if (verts[0].size() < 3) {
		Log::e() << "Triangulator received initial shape of less than 3 points";
		return 0;
	}

	// The maximum number of points you expect to need
	// This value is used by the library to calculate
	// working memory required
	uint32_t MaxPointCount = 10000;

	// Request how much memory (in bytes) you should
	// allocate for the library
	size_t MemoryRequired = MPE_PolyMemoryRequired(MaxPointCount);

	// Allocate a void* memory block of size MemoryRequired
	// IMPORTANT: The memory must be zero initialized
	void *Memory = calloc(MemoryRequired, 1);

	// Initialize the poly context by passing the memory pointer,
	// and max number of points from before
	MPEPolyContext PolyContext;
	if (!MPE_PolyInitContext(&PolyContext, Memory, MaxPointCount)) {
		Log::e() << "ERROR - couldn't start Poly2Tri";
		free(Memory);
		return 0;
	}

	{
		int i = 0;
		for (auto v: verts[0]) {
			MPEPolyPoint *Point = MPE_PolyPushPoint(&PolyContext);
			Point->X = v.x;
			Point->Y = v.y;
			Point->index = i;
			Point->array = 0;
			i++;
		}
	}

	MPE_PolyAddEdge(&PolyContext);

	// If you want to add holes to the shape you can do:
	for (int i = 1; i < verts.size(); i++) {
		if (verts[0].size() < 3) {
			Log::e() << "Triangulator received another shape that wasn't valid";
			return 0;
		}
		for (int j = 0; j < verts[i].size(); j++) {
			MPEPolyPoint *Hole = MPE_PolyPushPoint(&PolyContext);
			Hole->X = verts[i][j].x;
			Hole->Y = verts[i][j].y;
			Hole->index = j;
			Hole->array = i;
		}

		MPE_PolyAddEdge(&PolyContext);
	}

	// Triangulate the shape
	MPE_PolyTriangulate(&PolyContext);
	auto startIndex = outVerts.size();
	vector<unsigned int> offsets;
	for (int i = 0; i < verts.size(); i++) {
		outVerts.insert(outVerts.end(), verts[i].begin(), verts[i].end());
		if (i == 0) offsets.push_back(0);
		else
			offsets.push_back(offsets[i - 1] + (unsigned int) verts[i - 1].size());
	}

	// The resulting triangles can be used like so
	for (uxx i = 0; i < PolyContext.TriangleCount; ++i) {
		MPEPolyTriangle *Triangle = PolyContext.Triangles[i];
		MPEPolyPoint *PointA = Triangle->Points[0];
		MPEPolyPoint *PointB = Triangle->Points[1];
		MPEPolyPoint *PointC = Triangle->Points[2];
		indices.push_back((int) startIndex + PointA->index + offsets[PointA->array]);
		indices.push_back((int) startIndex + PointB->index + offsets[PointB->array]);
		indices.push_back((int) startIndex + PointC->index + offsets[PointC->array]);
	}
	free(Memory);
	return (int) (outVerts.size() - startIndex);
}
