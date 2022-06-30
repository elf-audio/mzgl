

#include "Drawer.h"
#include "RoundedRect.h"
#include "maths.h"
#include "util.h"

using namespace std;

void Drawer::setColor(glm::vec4 c) {
	this->color = c;
	isDoingGradient = false;
}

void Drawer::setColor(glm::vec4 c, float alpha) {
	this->color = c;
	color.a = alpha;
	isDoingGradient = false;
}

bool Drawer::isEmpty() {
	return geom.verts.size()==0;
}
void Drawer::fill() {
	filled = true;
}

void Drawer::noFill() {
	filled = false;
}

void Drawer::setColor(float grey) {
	setColor({grey, grey, grey, 1});
	isDoingGradient = false;
}



void Drawer::setGradient(vec4 c1, vec2 pos1, vec4 c2, vec2 pos2) {
	gradientStartColor = c1;
	gradientStopColor = c2;
	gradientStartPoint = pos1;
	gradientStopPoint = pos2;
	isDoingGradient = true;
	v2 = gradientStopPoint - gradientStartPoint;
	v2_ls = v2.x * v2.x + v2.y * v2.y;
}

//
//
// // projection of vector v1 onto v2
// static vec2 project( const vec2& v1, const vec2& v2 ) {
//	 float v2_ls = v2.x * v2.x + v2.y * v2.y;
//	 return v2 * ( dot( v2, v1 )/v2_ls );
//
// }
//vec4 Drawer::lookupGradient(vec2 pos) {
//	vec2 v2 = gradientStopPoint - gradientStartPoint;
//	vec2 v1 = pos - gradientStartPoint;
//	vec2 proj = gradientStartPoint + project(v1, v2);
//
//	if(gradientStartPoint.x==gradientStopPoint.x) { // vertical
//		float amt = mapf(proj.y, gradientStartPoint.y, gradientStopPoint.y, 0, 1, true);
//		return gradientStartColor * (1.f - amt) + gradientStopColor * amt;
//	} else {
//		float amt = mapf(proj.x, gradientStartPoint.x, gradientStopPoint.x, 0, 1, true);
//		return gradientStartColor * (1.f - amt) + gradientStopColor * amt;
//	}
//}
//

vec4 Drawer::lookupGradient(vec2 pos) {
	vec2 v1 = pos - gradientStartPoint;
	vec2 proj = gradientStartPoint + v2 * ( dot( v2, v1 )/v2_ls );

	if(gradientStartPoint.x==gradientStopPoint.x) { // vertical
		float amt = mapf(proj.y, gradientStartPoint.y, gradientStopPoint.y, 0, 1, true);
		return gradientStartColor * (1.f - amt) + gradientStopColor * amt;
	} else {
		float amt = mapf(proj.x, gradientStartPoint.x, gradientStopPoint.x, 0, 1, true);
		return gradientStartColor * (1.f - amt) + gradientStopColor * amt;
	}
}


void Drawer::setColor(float r, float g, float b, float a) {
	setColor({r, g, b, a});
	isDoingGradient = false;
}



void Drawer::drawTriangle(glm::vec2 a, glm::vec2 b, glm::vec2 c) {
	
    uint32_t s = (uint32_t) geom.verts.size();
	
    geom.verts.push_back(a); geom.verts.push_back(b); geom.verts.push_back(c);
	geom.indices.push_back(s); geom.indices.push_back(s+1); geom.indices.push_back(s+2);
	geom.cols.push_back(color); geom.cols.push_back(color); geom.cols.push_back(color);
}



// triangle with colours
void Drawer::drawTriangle(glm::vec2 a, glm::vec2 b, glm::vec2 c, glm::vec4 ca, glm::vec4 cb, glm::vec4 cc) {
    
    uint32_t s = (uint32_t) geom.verts.size();
    
	geom.verts.push_back(a); geom.verts.push_back(b); geom.verts.push_back(c);
	geom.indices.push_back(s); geom.indices.push_back(s+1); geom.indices.push_back(s+2);
	geom.cols.push_back(ca); geom.cols.push_back(cb); geom.cols.push_back(cc);
}

void Drawer::drawQuad(glm::vec2 a, glm::vec2 b, glm::vec2 c, glm::vec2 d) {
	drawTriangle(a, b, c);
	drawTriangle(c, d, a);
}



void Drawer::drawLine(glm::vec2 a, glm::vec2 b) {
	lineDrawer.thickness = strokeWeight;
	int numVerts = lineDrawer.getVerts({a,b}, geom.verts, geom.indices);
	geom.cols.insert(geom.cols.end(), numVerts, color);
}
void Drawer::drawLine(float ax, float ay, float bx, float by) {
	drawLine({ax, ay}, {bx, by});
}

void Drawer::drawLineStrip(const vector<vec2> &strip) {
    lineDrawer.thickness = strokeWeight;
    int numVerts = lineDrawer.getVerts(strip, geom.verts, geom.indices);
    geom.cols.insert(geom.cols.end(), numVerts, color);
}

void Drawer::drawLineStrip(const vector<vec2> &strip, const vector<vec4> &cols) {
    lineDrawer.thickness = strokeWeight;
    int numVerts = lineDrawer.getVerts(strip, geom.verts, geom.indices);
    for(const auto & a : cols) {
        geom.cols.push_back(a);
        geom.cols.push_back(a);
    }
    geom.cols.insert(geom.cols.end(), cols.begin(), cols.end());
}


void Drawer::drawTriangleStrip(const vector<vec2> &strip) {

	// size must be even and at least 4 verts
	mzAssert(strip.size()>=4 && strip.size()%2==0);
	// new way
	auto startPos = geom.verts.size();
	
//	geom.verts.reserve(geom.verts.size() + strip.size());
	geom.verts.insert(geom.verts.end(), strip.begin(), strip.end());

	auto numParts = strip.size()/2;
	for(uint32_t i = 1; i < numParts; i++) {
		
		auto ind = i*2 + (uint32_t)startPos;
		vector<uint32_t> indices = {
			ind - 2,
			ind,
			ind - 1,

			ind,
			ind + 1,
			ind - 1
		};
		geom.indices.insert(geom.indices.end(), indices.begin(), indices.end());
	}
	geom.cols.insert(geom.cols.end(), strip.size(), color);

}











void Drawer::drawRect(const Rectf &r) {
	vector<glm::vec2> v = {r.tl(), r.tr(), r.br(), r.bl()};
	if(filled) {
		uint32_t n = (uint32_t)geom.verts.size();
		vector<unsigned int> i = {n, n+1, n+2, n+2, n+3, n};
		geom.verts.insert(geom.verts.end(), v.begin(), v.end());
		geom.indices.insert(geom.indices.end(), i.begin(), i.end());
		if(isDoingGradient) {
			for(const auto &ve : v) {
				geom.cols.push_back(lookupGradient(ve));
			}
		} else {
			geom.cols.insert(geom.cols.end(), 4, color);
		}
	} else {
		lineDrawer.thickness = strokeWeight;
		int numVerts = lineDrawer.getVerts(v, geom.verts, geom.indices, true);
		geom.cols.insert(geom.cols.end(), numVerts, color);
	}
}


void Drawer::drawPlus(vec2 c, int diameter, int thickness) {
	
	Rectf r;
	r.setFromCentre(c, diameter, thickness);
	drawRect(r);
	r.setFromCentre(c, thickness, diameter);
	drawRect(r);
}


void Drawer::drawCross(vec2 c, int diameter, int thickness) {
	float s2 = 0.707106781186548;
	float p = diameter * s2 * 0.5f;
	float t = thickness * s2 * 0.5f;
	
	drawQuad(c + vec2(p-t, -p-t), c + vec2(p+t, -p+t), c + vec2(-p+t, p+t), c + vec2(-p-t, p-t));
	drawQuad(c + vec2(-p-t, -p+t), c + vec2(-p+t, -p-t), c + vec2(p+t, p-t), c + vec2(p-t, p+t));
}

void Drawer::drawChevronRight(vec2 c, int radius, int thickness) {
	float s2 = 0.707106781186548;
	float p = radius * s2;
	float t = thickness * s2;
	
	drawQuad(c, c + vec2(-t, t), c + vec2(-p-t, t-p), c+ vec2(-p, -p));
	drawQuad(c, c + vec2(-p, p), c + vec2(-p-t, p-t), c + vec2(-t, -t));
}


void Drawer::drawChevronLeft(vec2 c, int radius, int thickness) {
	float s2 = 0.707106781186548;
	float p = radius * s2;
	float t = thickness * s2;
	
	drawQuad(c, c + vec2(t, t), c + vec2(p+t, t-p), c+ vec2(p, -p));
	drawQuad(c, c + vec2(p, p), c + vec2(p+t, p-t), c + vec2(t, -t));
}

void Drawer::drawChevronDown(vec2 c, int radius, int thickness) {
	float s2 = 0.707106781186548;
	float p = radius * s2;
	float t = thickness * s2;
	
	drawQuad(c, c + vec2(-p, -p), c + vec2(-p+t, -t-p), c+ vec2(t, -t));
	drawQuad(c, c + vec2(-t, -t), c + vec2(p-t, -p-t), c + vec2(p, -p));
}
void Drawer::drawChevronUp(vec2 c, int radius, int thickness) {
	float s2 = 0.707106781186548;
	float p = radius * s2;
	float t = thickness * s2;
	
	drawQuad(c, c + vec2(-p, p), c + vec2(-p+t, t+p), c+ vec2(t, t));
	drawQuad(c, c + vec2(-t, t), c + vec2(p-t, p+t), c + vec2(p, p));
}



void Drawer::drawCircle(glm::vec2 c, float r) {
	
    auto startIndex = geom.verts.size();
	if(filled) {
		
		for(float th = 0; th < M_PI * 2; th += M_PI*0.02) {
			geom.verts.push_back({c.x + cos(th)*r, c.y + sin(th)*r});
		}
		
		auto numVerts = geom.verts.size() - startIndex;
		geom.cols.insert(geom.cols.end(), numVerts, color);
		for(int i = 0; i < numVerts - 2; i++) {
			geom.indices.push_back((uint32_t)startIndex);
			geom.indices.push_back((uint32_t)startIndex+i+1);
			geom.indices.push_back((uint32_t)startIndex+i+2);
		}
	} else {
		float sw = strokeWeight*0.5;
		float inner = r - sw;
		float outer = r + sw;
		for(float th = 0; th < M_PI * 2; th += M_PI*0.02) {
			float x = cos(th); float y = sin(th);
			geom.verts.push_back({c.x + x*outer, c.y + y*outer});
			geom.verts.push_back({c.x + x*inner, c.y + y*inner});
		}
		
		auto numVerts = geom.verts.size() - startIndex;
		
		geom.cols.insert(geom.cols.end(), numVerts, color);
		
		for(int i = 0; i < numVerts; i+=2) {
			geom.indices.push_back(startIndex + i);
			geom.indices.push_back(startIndex + (i + 2)%numVerts);
			geom.indices.push_back(startIndex + (i + 1)%numVerts);
			
			geom.indices.push_back(startIndex + (i + 2)%numVerts);
			geom.indices.push_back(startIndex + (i + 3)%numVerts);
			geom.indices.push_back(startIndex + (i + 1)%numVerts);
		}
	}
	
	
}

void Drawer::drawRect(float x, float y, float width, float height) {
	drawRect({x, y, width, height});
}

void Drawer::drawRoundedRect(const Rectf &r, float radius) {
	if(radius<4.1f) radius = 4.1f;
	rrv.clear();
	::getPerfectRoundedRectVerts(r, radius, rrv);
	
	
	if(filled) {
		auto start = geom.verts.size();
		geom.verts.insert(geom.verts.end(), rrv.begin(), rrv.end());
		for(unsigned int i = 0; i < rrv.size()-2; i++) {
			geom.indices.push_back(start);
			geom.indices.push_back(start + i + 1);
			geom.indices.push_back(start + i + 2);
		}
		if(isDoingGradient) {
			geom.cols.reserve(geom.cols.size() + rrv.size());
			for(const auto &v : rrv) {
				geom.cols.push_back(lookupGradient(v));
			}
		} else {
			geom.cols.insert(geom.cols.end(), rrv.size(), color);
		}
	} else {
		rrv.pop_back();
		lineDrawer.thickness = strokeWeight;
		int numVerts = lineDrawer.getVerts(rrv, geom.verts, geom.indices, true);
		
		geom.cols.insert(geom.cols.end(), numVerts, color);
	}
}

void Drawer::drawRoundedRectShadow(const Rectf &r, float radius, float shadow) {
	if(radius<4.1f) radius = 4.1f;
	rrv.clear();
	::getPerfectRoundedRectVerts(r, radius, rrv);
	
	rrv.pop_back();
	lineDrawer.outside = true;
	lineDrawer.thickness = shadow;
	Geometry geom;
	int numVerts = lineDrawer.getVerts(rrv, geom.verts, geom.indices, true);
	geom.cols.reserve(geom.cols.size() + numVerts);
	
	auto c = color;
	c.a = 0.f;
	for(int i = 0; i < numVerts; i+=2) {
		
		geom.cols.push_back(color);
		geom.cols.push_back(c);
	}
}


void Drawer::commit(VboRef vbo, bool ignoreColor, bool addNormalizedTexCoords) {
	if(geom.verts.size()>0) {
		vbo->setVertices(geom.verts);
		if(!ignoreColor) vbo->setColors(geom.cols);
		vbo->setIndices(geom.indices);
		if(addNormalizedTexCoords) {
			geom.calculateNormalizedTexCoords();
			vbo->setTexCoords(geom.texCoords);
		}
		vbo->setMode(GL_TRIANGLES);
	}
	geom.clear();
}


VboRef Drawer::createVbo(bool ignoreColor, bool addNormalizedTexCoords) {
	VboRef vbo = Vbo::create();
	commit(vbo, ignoreColor, addNormalizedTexCoords);
	return vbo;
}
void Drawer::addGeometry(Geometry &_geom) {
	int firstIndex = geom.verts.size();
	geom.verts.insert(geom.verts.end(), _geom.verts.begin(), _geom.verts.end());
	geom.cols.insert(geom.cols.end(), _geom.cols.begin(), _geom.cols.end());
	
	
//	// old way
//	vector<unsigned int> ind = _geom.indices;
//	for(int i = 0; i < ind.size(); i++) ind[i] += firstIndex;
//	geom.indices.insert(geom.indices.end(), ind.begin(), ind.end());
	
	// new way
	int startPos = geom.indices.size();
	// this might speed up the insert
	geom.indices.reserve(geom.indices.size() + _geom.indices.size());
	geom.indices.insert(geom.indices.end(), _geom.indices.begin(), _geom.indices.end());
	for(int i = startPos; i < geom.indices.size(); i++) {
		geom.indices[i] += firstIndex;
	}
}




void Drawer::getPerfectRoundedRectVerts(const Rectf &r, float radius, vector<glm::vec2> &outVerts
								, bool tl, bool tr, bool br, bool bl
								) {
	vector<glm::vec2> cache;
	float pixelsPerStep = 2;
	float ang = pixelsPerStep / 2.f / radius;
	if(ang>1) {
		outVerts.push_back(r.tl());
		outVerts.push_back(r.tr());
		outVerts.push_back(r.br());
		outVerts.push_back(r.bl());
		return;
	}
	float step = asin(pixelsPerStep / 2.f / radius) / 2.f;
	float numSteps = M_PI*2.f / step;
	
	createRoundedRectCache(cache, numSteps);
	roundedRectVerts(r, radius, outVerts, cache, tl, tr, br,bl);
}

void Drawer::drawRoundedRect(const Rectf &r, float radius, bool tl, bool tr, bool br, bool bl) {
	if(radius<4.1) radius = 4.1f;
	rrv.clear();
	getPerfectRoundedRectVerts(r, radius, rrv, tl, tr, br, bl);
	
	if(filled) {
		unsigned int start = geom.verts.size();
		geom.verts.insert(geom.verts.end(), rrv.begin(), rrv.end());
		for(unsigned int i = 0; i < rrv.size()-2; i++) {
			geom.indices.push_back(start);
			geom.indices.push_back(start + i + 1);
			geom.indices.push_back(start + i + 2);
		}
		if(isDoingGradient) {
			geom.cols.reserve(geom.cols.size() + rrv.size());
			for(const auto &v : rrv) {
				geom.cols.push_back(lookupGradient(v));
			}
		} else {
			geom.cols.insert(geom.cols.end(), rrv.size(), color);
		}
	} else {
		rrv.pop_back();
		lineDrawer.thickness = strokeWeight;
		int numVerts = lineDrawer.getVerts(rrv, geom.verts, geom.indices, true);
		
		geom.cols.insert(geom.cols.end(), numVerts, color);
	}
}




void Drawer::roundedRectVerts(const Rectf &r, float radius, vector<glm::vec2> &outVerts, vector<glm::vec2> &cache
					  , bool tl, bool tr, bool br, bool bl
					  
					  ) {
	// top left
	auto t = r.tl();
	
	if(tl) {
		for(int i = 0; i < cache.size(); i++) {
			outVerts.push_back(t + cache[i] * radius);
		}
	} else {
		outVerts.push_back(t);
	}
	
	// top right
	t = r.tr();
	if(tr) {
		for(auto i = cache.size(); i --> 0;) {
			outVerts.push_back(t + cache[i] * glm::vec2(-radius, radius));
		}
	} else {
		outVerts.push_back(t);
	}
	
	// bottom right
	t = r.br();
	if(br) {
		for(int i = 0; i < cache.size(); i++) {
			outVerts.push_back(t - cache[i] * radius);
		}
	} else {
		outVerts.push_back(t);
	}
	// bottom left
	t = r.bl();
	if(bl) {
		for(auto i = cache.size(); i --> 0;) {
			outVerts.push_back(t + cache[i] * glm::vec2(radius, -radius));
		}
	} else {
		outVerts.push_back(t);
	}
	
	// close the shape
	outVerts.push_back(r.tl() + cache[0] * radius);
}


void Drawer::createRoundedRectCache(vector<glm::vec2> &cache, int numSteps) {
	cache.resize(numSteps); //20
	for(int i = 0; i < cache.size(); i++) {
		float phi = mapf(i, 0, cache.size(), M_PI, M_PI + M_PI/2.0);
		cache[i] = glm::vec2(1.f + (float)cos(phi), 1.f + (float)sin(phi));
	}
}
