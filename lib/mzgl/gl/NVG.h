//
//  NVG.h
//  mzgl
//
//  Created by Marek Bereza on 18/05/2020.
//  Copyright © 2020 Marek Bereza. All rights reserved.
//

#pragma once


class Graphics;
#include "nanovg.h"
class NVG {
public:
	void begin(Graphics &g);
	void end();
	
	void stroke();
	void stroke(vec4 c);
	void stroke(float bri);
	void noStroke();
	void strokeGradientLinear(vec4 colA, vec4 colB, vec2 posA, vec2 posB);
	void strokeGradientRadial(vec4 colA, vec4 colB, vec2 centre, float innerRadius, float outerRadius);
	void strokeGradientBox(vec4 colA, vec4 colB, Rectf rect, float radius, float feather);
	
	void fill();
	void fill(vec4 c);
	void fill(float bri);
	void fillGradientLinear(vec4 colA, vec4 colB, vec2 posA, vec2 posB);
	void fillGradientRadial(vec4 colA, vec4 colB, vec2 centre, float innerRadius, float outerRadius);
	void fillGradientBox(vec4 colA, vec4 colB, Rectf rect, float radius, float feather);
	
	void noFill();
	
	
	void strokeWidth(float x);
	
	void roundCaps();
	void buttCaps();
	void squareCaps();
	
	void roundJoins();
	void buttJoins();
	void squareJoins();
	
	
	void mitreLimit(float limit);
	
	void beginScissor(const Rectf &r);
	void endScissor();
	
	
	
	
	
	void drawRect(const Rectf &r);
	void drawRoundedRect(const Rectf &r, float radius);
	void drawRoundedRect(const Rectf &r, float tlr, float trr, float brr, float blr);


	void drawEllipse(const Rectf &r);
	void drawCircle(vec2 c, float r);
	
	void drawTriangle(vec2 a, vec2 b, vec2 c);
	void drawLine(vec2 a, vec2 b);
	
	void setDefaultFont(std::string path, float size);
	void drawText(std::string text, vec2 pos);
	void drawText(std::string text, vec2 pos, int alignHoriz, int alignVert);
	void drawTextCentred(std::string text, vec2 pos);
	
	void drawLineStrip(const std::vector<vec2> &v);
	void drawLines(const std::vector<vec2> &v);
	virtual ~NVG();

private:
	
	Graphics *graphics;
	void init();
	void prepareDefaultFont();
	bool filling = false;
	bool stroking = false;
	bool inited = false;
	NVGcontext *vg;
	vec4 fillColor;
	vec4 strokeColor;
	void stylePath();
	int defaultFont = -1;
	float defaultFontSize = 32;
};
