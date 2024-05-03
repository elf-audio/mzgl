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

enum class HTextAlign { Left, Centre, Right };
enum class VTextAlign { Top, Centre, Bottom, Baseline };

class Font {
public:
	bool load(std::string path, float fontSize);
	bool load(const std::vector<unsigned char> &data, float fontSize);

	void draw(Graphics &g, const std::string &text, float x, float y);
	void draw(Graphics &g, const std::string &text, glm::vec2 c);
	void draw(Graphics &g, const std::string &text, glm::vec2 c, HTextAlign halign, VTextAlign valign);
	void addVerts(const std::string &text,
				  glm::vec2 c,
				  std::vector<glm::vec2> &verts,
				  std::vector<glm::vec2> &uvs,
				  HTextAlign halign,
				  VTextAlign valign);

	struct VerticalMetrics {
		float ascender;
		float descender;
		float textHeight;
		float lineHeight;
	};
	VerticalMetrics getVerticalMetrics();

	void
		addVerts(const std::string &text, glm::vec2 c, std::vector<glm::vec2> &verts, std::vector<glm::vec2> &uvs);

	void drawCentred(Graphics &g, const std::string &text, glm::vec2 c);
	void drawVerticallyCentred(Graphics &g,
							   const std::string &text,
							   glm::vec2 c,
							   HTextAlign align = HTextAlign::Left);
	void drawHorizontallyCentred(Graphics &g, const std::string &text, glm::vec2 c);

	// draws the string making c the bottom left corner - get rid of this in favour of draw(..., halign, valign)
	void drawBottomLeftAligned(Graphics &g, const std::string &text, glm::vec2 c);

	std::string ellipsize(const std::string &t, int w) const;
	Rectf getRect(const std::string &text, float x, float y) const;
	Rectf getRect(const std::string &text, glm::vec2 c) const;
	glm::vec2 getDims(const std::string &text) const;
	float getWidth(const std::string &text) const;
	float getHeight(const std::string &text) const;

	std::vector<std::string> wrapText(const std::string &text, float width) const;

	TextureRef getAtlasTexture();

	Font();
	virtual ~Font();

	bool isLoaded() const;

	void clear();

	// you can draw a font smaller than the texture atlas is set to
	// if you want... use with care, and remember to put it back to
	// 1 when you're not using it.
	void setScale(float scale);

	// should be private
	void fontstashError(int error, int val);

#ifdef __ANDROID__
	static std::vector<Font *> fonts;
#endif
private:
	VerticalMetrics verticalMetrics;
	FONScontext *fs = nullptr;
	int fontNormal	= -1;
	float size		= 0;
	float scale		= 1;
};
