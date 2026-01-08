//
//  RoundedRect.cpp
//  linerender
//
//  Created by Marek Bereza on 28/02/2018.
//  Copyright © 2018 Marek Bereza. All rights reserved.
//

#include "RoundedRect.h"
#include "maths.h"
#include "Graphics.h"
#include "MitredLine.h"
#include "Drawer.h"
using namespace std;

vector<glm::vec2> roundedRectCache;
void createRoundedRectCache(vector<glm::vec2> &cache, int numSteps) {
	cache.resize(numSteps); //20
	for (size_t i = 0; i < cache.size(); i++) {
		float phi = mapf(i, 0, cache.size(), M_PI, M_PI + M_PI / 2.f);
		cache[i]  = glm::vec2(1.f + (float) cos(phi), 1.f + (float) sin(phi));
	}
}

VboRef makeRoundedRectVbo(const Rectf &r, float radius, bool solid, float strokeWeight) {
	vector<glm::vec2> verts;
	getRoundedRectVerts(r, radius, verts);

	if (solid) {
		Drawer d;
		d.setColor(1);
		d.drawTriangleFan(verts);
		return d.createVbo();

	} else {
		auto m = Vbo::create();
		MitredLine lineDrawer;

		verts.pop_back();
		lineDrawer.thickness = strokeWeight;

		vector<vec2> vs;
		vector<uint32_t> indices;

		lineDrawer.getVerts(verts, vs, indices, MitredLine::OpenOrClosed::Closed);

		m->setVertices(vs);
		m->setIndices(indices);

		m->setMode(Vbo::PrimitiveType::TriangleStrip);
		return m;
	}
}

void roundedRectVerts(const Rectf &r, float radius, vector<glm::vec2> &outVerts, vector<glm::vec2> &cache) {
	// top left
	auto t = r.tl();

	for (int i = 0; i < cache.size(); i++) {
		outVerts.push_back(t + cache[i] * radius);
	}

	// top right
	t = r.tr();
	for (int i = (int) cache.size() - 1; i >= 0; i--) {
		outVerts.push_back(t + cache[i] * glm::vec2(-radius, radius));
	}

	// bottom right
	t = r.br();
	for (int i = 0; i < cache.size(); i++) {
		outVerts.push_back(t - cache[i] * radius);
	}
	// bottom left
	t = r.bl();
	for (int i = (int) cache.size() - 1; i >= 0; i--) {
		outVerts.push_back(t + cache[i] * glm::vec2(radius, -radius));
	}

	// close the shape
	outVerts.push_back(r.tl() + cache[0] * radius);
}

void getPerfectRoundedRectVerts(const Rectf &r, float radius, vector<glm::vec2> &outVerts) {
	if (radius < 1) {
		outVerts.push_back(r.tl());
		outVerts.push_back(r.tr());
		outVerts.push_back(r.br());
		outVerts.push_back(r.bl());

	} else {
		vector<glm::vec2> cache;
		float pixelsPerStep = 8;
		float ang			= pixelsPerStep / 2.f / radius;
		if (ang > 1) {
			outVerts.push_back(r.tl());
			outVerts.push_back(r.tr());
			outVerts.push_back(r.br());
			outVerts.push_back(r.bl());
			return;
		}

		float step	   = asin(ang) / 2.f;
		float numSteps = M_PI * 2.f / step;

		//		printf("num steps: %.0f\n", numSteps);
		createRoundedRectCache(cache, numSteps);
		roundedRectVerts(r, radius, outVerts, cache);
	}
}

void getRoundedRectVerts(const Rectf &r, float radius, vector<glm::vec2> &outVerts) {
	if (roundedRectCache.size() == 0) {
		createRoundedRectCache(roundedRectCache, 20);
	}

	roundedRectVerts(r, radius, outVerts, roundedRectCache);
}

void RoundedRect::touch() {
	oldRect.width = 0.f;
}

void RoundedRect::fill(Graphics &g, const Rectf &r, float radius) {
	if (mesh == nullptr || r != oldRect || !oldSolid || radius != oldRadius) {
		oldRect	  = r;
		oldSolid  = true;
		oldRadius = radius;
		mesh	  = makeRoundedRectVbo(r, radius, true);
	}
	mesh->draw(g);
}
void RoundedRect::stroke(Graphics &g, const Rectf &r, float radius, float strokeWeight) {
	if (mesh == nullptr || r != oldRect || oldSolid || radius != oldRadius || strokeWeight != oldStrokeWeight) {
		oldRect			= r;
		oldSolid		= true;
		oldRadius		= radius;
		oldStrokeWeight = strokeWeight;
		mesh			= makeRoundedRectVbo(r, radius, false, strokeWeight);
	}
	mesh->draw(g);
}
