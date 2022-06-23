//
//  Font.hpp
//  MZGL
//
//  Created by Marek Bereza on 15/01/2018.
//  Copyright © 2018 Marek Bereza. All rights reserved.
//

#pragma once


#include <string>
#include "Texture.h"
struct FONScontext;

enum class TextAlign {
	Left,
	Center,
	Right
};

class Font {
public:
	bool load(std::string path, float fontSize);
	bool load(const std::vector<unsigned char> &data, float fontSize);
	
	void draw(Graphics &g, const std::string &text, float x, float y);
	void draw(Graphics &g, const std::string &text, glm::vec2 c);
	void drawCentred(Graphics &g, const std::string &text, glm::vec2 c);
	void drawVerticallyCentred(Graphics &g, const std::string &text, glm::vec2 c, TextAlign align = TextAlign::Left);
	void drawHorizontallyCentred(Graphics &g, const std::string &text, glm::vec2 c);
	
	// draws the string making c the bottom left corner
	void drawBottomLeftAligned(Graphics &g, const std::string &text, glm::vec2 c);
	
	std::string ellipsize(const std::string &t, int w) const;
	Rectf getRect(const std::string &text, float x, float y) const;
	Rectf getRect(const std::string &text, glm::vec2 c) const;
	glm::vec2  getDims(const std::string &text) const;
	float getWidth(const std::string &text) const;
	float getHeight(const std::string &text) const;
	TextureRef getAtlasTexture();
	
	Font();
	virtual ~Font();
	
	bool isLoaded() const;
	
	void clear();

	// should be private
	void fontstashError(int error, int val);

#ifdef __ANDROID__
	static std::vector<Font*> fonts;
#endif
private:
	FONScontext* fs = nullptr;
	int fontNormal = -1;
	float size = 0;
};

