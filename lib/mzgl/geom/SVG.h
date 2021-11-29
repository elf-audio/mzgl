//
//  SVG.h
//  roundedtexrect
//
//  Created by Marek Bereza on 02/03/2018.
//  Copyright © 2018 Marek Bereza. All rights reserved.
//

#pragma once

#include <glm/glm.hpp>
#include <memory>

#include "Graphics.h"
#include <map>

#define SVG_CIRCLE_RESOLUTION 3
#define SVG_CUBIC_RESOLUTION 1
namespace pugi {
	class xml_node;
};

struct SVGState {
	float opacity = 1;
	glm::mat3 transform;
	
	bool filling = false;
	glm::vec3 fillColor;
	
	bool stroking = false;
	glm::vec3 strokeColor;
	
	float strokeWeight = 1;
	float fillOpacity = 1;
	float strokeOpacity = 1;
	
	SVGState() {
		transform = {1,0,0,0,1,0,0,0,1};
	}
};
class SVGShape;
typedef std::shared_ptr<SVGShape> SVGShapeRef;

class SVGNode;
typedef std::shared_ptr<SVGNode> SVGNodeRef;

class SVGGroup;
typedef std::shared_ptr<SVGGroup> SVGGroupRef;

class SVGNode {
public:
	std::vector<SVGNodeRef> children;
	
	
	virtual void getOutlines(std::vector<glm::vec2> &outlines) = 0;
	virtual void getTriangles(std::vector<glm::vec2> &verts, std::vector<glm::vec4> &cols, std::vector<unsigned int> &indices) = 0;
	
	virtual void applyState(SVGState state = SVGState()) = 0;
	virtual void scale(float s) = 0;
	virtual void mirrorX() = 0;
	virtual void mirrorY() = 0;
	virtual void rotate(float theta) = 0;
	virtual void translate(float x, float y) = 0;
};


class SVGDoc {
public:
	bool loadFromString(const std::string &svgData);
	bool load(std::string path);
	void scale(float s);
	void mirrorX();
	void mirrorY();
	void rotate(float theta);
	void translate(float x, float y);
	void getTriangles(std::vector<glm::vec2> &verts, std::vector<glm::vec4> &cols, std::vector<unsigned int> &indices);
	void getOutlines(std::vector<glm::vec2> &verts);
	float width = 0;
	float height = 0;
private:
	Rectf viewBox;
	std::map<std::string,SVGShapeRef> defs;
	SVGGroupRef rootGroup;
	
	void parseViewBox(std::string s);
	void parseDefs(const pugi::xml_node &root);
	void parse(const pugi::xml_node &n, int depth = 0);
	
};
