//
//  Vbo.h
//  MZGL
//
//  Created by Marek Bereza on 15/01/2018.
//  Copyright © 2018 Marek Bereza. All rights reserved.
//

#pragma once




#include "mzOpenGL.h"

#define DO_DRAW_STATS




#include <glm/glm.hpp>
// TODO: this should go somewhere more global
using vec2 = glm::vec2;
using vec3 = glm::vec3;
using vec4 = glm::vec4;

#include <vector>
#include <memory>




class Vbo;
typedef std::shared_ptr<Vbo> VboRef;
class Graphics;
class Geometry;

class Vbo {
public:
	GLuint vertexArrayObject = 0;
	GLuint vertexBuffer = 0;
	GLuint colorbuffer = 0;
	GLuint texCoordBuffer = 0;
	GLuint normalBuffer = 0;
	GLuint indexBuffer = 0;
	
	static VboRef create() {
		return VboRef(new Vbo());
	}
	
	virtual ~Vbo();

#ifdef __ANDROID__
	static std::vector<Vbo*> vbos;
	static void printVbos();
#endif
	void clear();
	
	Vbo &setVertices(const std::vector<vec4> &verts);
	Vbo &setVertices(const std::vector<vec3> &verts);
	Vbo &setVertices(const std::vector<vec2> &verts);
	
	Vbo &setTexCoords(const std::vector<vec2> &tcs);
	
	Vbo &setColors(const std::vector<vec3> &cols);
	Vbo &setColors(const std::vector<vec4> &cols);
	
	Vbo &setNormals(const std::vector<vec3> &norms);
	Vbo &setIndices(const std::vector<unsigned int> &indices);
	
	Vbo &setGeometry(const Geometry &geom);
	//void addNormalizedTexCoords();
	
	
	// GL_TRIANGLES etc optional
	Vbo &setMode(GLuint mode);
	
	void draw(Graphics &g, int mode = -1, size_t instances = 1);
	void drawInstanced(Graphics &g, size_t instances) { draw(g, -1, instances);};
	size_t getNumVerts() { return numVerts; }
	
#ifdef DO_DRAW_STATS
	static int numDrawnVerts;
	static int numDrawCalls;
	static void resetDrawStats();
#endif
private:
#ifdef DO_DRAW_STATS
	static int _numDrawnVerts;
	static int _numDrawCalls;
#endif
	
	GLuint mode = 0xDEADBEEF;
	
	size_t numVerts = 0;
	size_t numIndices = 0;
	size_t numCols = 0;
	
	size_t numTcs = 0;
	size_t numNorms = 0;
	
	int vertDimensions = 0;
	int colDimensions = 0;
	
	void generateVertBuffer(const float *data, size_t numVerts, int numDims);
	void updateVertBuffer(const float *data);
	
	void generateColorBuffer(const float *data, size_t numCols, int numDims);
	
	void deallocateResources();
	
private:
	Vbo();
};


