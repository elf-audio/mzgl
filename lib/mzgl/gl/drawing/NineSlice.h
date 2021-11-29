//
//  NineSlice.h
//  roundedtexrect
//
//  Created by Marek Bereza on 28/02/2018.
//  Copyright © 2018 Marek Bereza. All rights reserved.
//

#pragma once

typedef std::shared_ptr<Texture> TextureRef;

class NineSlice {
public:
	TextureRef tex;
	Vbo *vbo = nullptr;
	~NineSlice() {
		if(vbo!=nullptr) delete vbo;
	}
	void setup(TextureRef tex) {
		vbo = new Vbo();
		this->tex = tex;
		vbo->setTexCoords({
			{0,0}, 		{0.5,0}, 	{0.5,0}, 	{1,0},
			{0,0.5}, 	{0.5,0.5}, 	{0.5,0.5}, 	{1,0.5},
			{0,0.5}, 	{0.5,0.5}, 	{0.5,0.5}, 	{1,0.5},
			{0,1}, 		{0.5,1}, 	{0.5,1}, 	{1,1}
		});
		
		
		vbo->setIndices({
			0,1,5,5,4,0,
			1,2,6,6,5,1,
			2,3,6,3,7,6,
			4,5,9,9,8,4,
			5,6,10,10,9,5,
			6,7,11,11,10,6,
			8,9,12,9,13,12,
			9,10,14,14,13,9,
			10,11,15,15,14,10
			
		});
	}
	
	void draw(Rectf r) {
		float radius = tex->width / 2.f;
		
		vbo->setVertices({
			r.tl(),
			r.tl() + glm::vec2(radius, 0),
			r.tr() + glm::vec2(-radius, 0),
			r.tr(),
			
			
			r.tl() + glm::vec2(0, radius),
			r.tl() + glm::vec2(radius, radius),
			r.tr() + glm::vec2(-radius, radius),
			r.tr() + glm::vec2(0, radius),
			
			
			r.bl() + glm::vec2(0, -radius),
			r.bl() + glm::vec2(radius, -radius),
			r.br() + glm::vec2(-radius, -radius),
			r.br() + glm::vec2(0, -radius),
			
			r.bl(),
			r.bl() + glm::vec2(radius, 0),
			r.br() + glm::vec2(-radius, 0),
			r.br()
		});
		
		tex->bind();
		vbo->draw();
		tex->unbind();
	}
};

