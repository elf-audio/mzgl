//
//  Font.cpp
//  MZGL
//
//  Created by Marek Bereza on 15/01/2018.
//  Copyright © 2018 Marek Bereza. All rights reserved.
//

#include "Font.h"
#include <stdio.h> // malloc, free, fopen, fclose, ftell, fseek, fread
#include <string.h> // memset
#include "Shader.h"
#include "Graphics.h"
#define FONTSTASH_IMPLEMENTATION // Expands implementation

#include "mzOpenGL.h"
#include "fontstash.h"
#include "util.h"
#ifdef __APPLE__ // I don't think this block is needed
#	include <TargetConditionals.h>

#endif
#include "log.h"
#define GLFONTSTASH_IMPLEMENTATION // Expands implementation

#include "filesystem.h"
using namespace std;
#include "OpenGLShader.h"

#ifdef __ANDROID__
vector<Font *> Font::fonts;
#endif
#include "error.h"
/*
 This uses the version of fontstash on github as of 16/01/18
 changing this line:
 glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzleRgbaParams);
 
 for this:
 glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, swizzleRgbaParams[0]);
 glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, swizzleRgbaParams[1]);
 glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, swizzleRgbaParams[2]);
 glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, swizzleRgbaParams[3]);
 
 and also:
 
 glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, gl->width, gl->height, 0, GL_RED, GL_UNSIGNED_BYTE, NULL);
 
 for this:
 
 glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, gl->width, gl->height, 0, GL_RED, GL_UNSIGNED_BYTE, NULL);
 
 (Changing GL_RED internal format for GL_R8 - seems to work on both mac and iOS)
 */

#ifdef MZGL_SOKOL_METAL
#	include "sokolFontstash.h"

#else
#	include "gl3corefontstash.h"
#endif
#include "error.h"

using namespace std;

void fontstashErrorCallback(void *font, int error, int val) {
	Log::e() << "Got fontstash error " << error << " " << val;
	auto f = (Font *) font;
	f->fontstashError(error, val);
}

void Font::fontstashError(int error, int val) {
	// if there's no more space for
	// glyphs in the font atlas, make
	// it bigger
	if (error == FONS_ATLAS_FULL) {
		int width  = 0;
		int height = 0;
		fonsGetAtlasSize(fs, &width, &height);
		if (width < height) {
			fonsExpandAtlas(fs, width * 2, height);
		} else {
			fonsExpandAtlas(fs, width, height * 2);
		}
	}
}
void Font::setScale(float scale) {
	this->scale = scale;
}
bool Font::isLoaded() const {
	return fs != nullptr;
}

bool Font::load(Graphics &g, const vector<unsigned char> &data, float size) {
	// todo - this is just a copy of the next method

	clear();
	this->size = size / 2.f;
#ifdef MZGL_SOKOL_METAL
	fs = sokolFonsCreate(g, 512, 512, FONS_ZERO_TOPLEFT);
#else
	GetError();
	fs = glfonsCreate(512, 512, FONS_ZERO_TOPLEFT);
#endif
	fonsSetErrorCallback(fs, &fontstashErrorCallback, this);

	if (fs == nullptr) {
		Log::e() << "Could not create stash.";
		return false;
	}
	// have to make some memory here that will be freed by fontstash
	unsigned char *d = new unsigned char[data.size()];
	memcpy(d, data.data(), data.size());
	fontNormal = fonsAddFontMem(fs, "sans", d, data.size(), 1);
	if (fontNormal == FONS_INVALID) {
		Log::e() << "Could not add font normal.";
		return false;
	}
	// need tos et the font and size in order to make getRect work
	fonsSetSize(fs, (int) this->size);
	fonsSetFont(fs, fontNormal);

	fonsVertMetrics(fs, &verticalMetrics.ascender, &verticalMetrics.descender, &verticalMetrics.lineHeight);
	verticalMetrics.textHeight = verticalMetrics.ascender + verticalMetrics.descender;

	fonsSetErrorCallback(fs, &fontstashErrorCallback, this);

	return true;
}

bool Font::load(Graphics &g, string path, float size) {
	clear();

	this->size = size / 2.f;
#ifdef MZGL_SOKOL_METAL
	fs = sokolFonsCreate(g, 512, 512, FONS_ZERO_TOPLEFT);
#else
	fs = glfonsCreate(512, 512, FONS_ZERO_TOPLEFT);
#endif
	if (fs == nullptr) {
		Log::e() << "Could not create stash.";
		return false;
	}

	fontNormal = fonsAddFont(fs, "sans", path.c_str());
	if (fontNormal == FONS_INVALID) {
		Log::e() << "Could not add font normal.";
		return false;
	}
	// need tos et the font and size in order to make getRect work
	fonsSetSize(fs, this->size);
	fonsSetFont(fs, fontNormal);

	fonsVertMetrics(fs, &verticalMetrics.ascender, &verticalMetrics.descender, &verticalMetrics.lineHeight);
	verticalMetrics.textHeight = verticalMetrics.ascender + verticalMetrics.descender;

	return true;
}

TextureRef Font::getAtlasTexture(Graphics &g) {
#ifdef MZGL_SOKOL_METAL
	sokolFONScontext *sg = (sokolFONScontext *) (fs->params.userPtr);
	return Texture::create(g, sg->tex.id, sg->width, sg->height);
#else
	GLFONScontext *gl = (GLFONScontext *) (fs->params.userPtr);
	return Texture::create(g, gl->tex, gl->width, gl->height);
#endif
}

std::string Font::ellipsize(const std::string &str, int w) const {
	if (str.size() < 4 || getWidth(str) <= w) return str;

	auto s = str;

	auto centreIndex = s.size() / 2;
	auto front		 = s.substr(0, centreIndex - 2);
	auto back		 = s.substr(centreIndex + 1);

	bool flipFlop = false;
	while (front.size() > 1 && back.size() > 1) {
		s = front + "..." + back;
		if (getWidth(s) <= w) return s;
		if (flipFlop) back.erase(0, 1);
		else front.pop_back();
		flipFlop = !flipFlop;
	}
	return s;
}

Rectf Font::getRect(const string &text, float x, float y) const {
	if (fs == nullptr) {
		Log::e() << "Calling getRect on null Font";
		return Rectf();
	}
	float b[4];
	fonsTextBounds(fs, x, y, text.c_str(), nullptr, b);
	auto r = Rectf(b[0], b[1], b[2] - b[0], b[3] - b[1]);
	return r;
}

vec2 Font::getDims(const string &text) const {
	auto r = getRect(text, 0, 0);
	return {r.width, r.height};
}

float Font::getWidth(const string &text) const {
	return getRect(text, 0, 0).width;
}

float Font::getHeight(const string &text) const {
	return getRect(text, 0, 0).height;
}

Rectf Font::getRect(const string &text, glm::vec2 c) const {
	return getRect(text, c.x, c.y);
}

void Font::draw(Graphics &g, const string &text, glm::vec2 c) {
	draw(g, text, c.x, c.y);
}

void Font::draw(Graphics &g, const string &text, float x, float y) {
	if (fs == nullptr) {
		Log::e() << "Calling getRect on null Font";
		return;
	}

	if (scale != 1) {
		g.pushMatrix();
		g.translate(x, y);
		g.scale(scale);
	}

	auto shader = g.fontShader;

#ifdef MZGL_SOKOL_METAL
#else
	GLFONScontext *gl = (GLFONScontext *) (fs->params.userPtr);

	if (gl->VERTEX_ATTRIB == 0) {
		auto *glShader	  = dynamic_cast<OpenGLShader *>(shader.get());
		gl->VERTEX_ATTRIB = glShader->positionAttribute;
		gl->TCOORD_ATTRIB = glShader->texCoordAttribute;
	}
#endif
	shader->begin();

	shader->setMVP(g.getMVP());

#ifdef __ANDROID__
	// ok this is mega shit, but Huawei phones, when they receive an alpha of 1 here
	// for some reason, they blow up and set the whole texture alpha to 1, so
	// this code just turns it shy of 1
	auto c = g.getColor();
	if (c.a == 1.f) c.a = 0.999f;
	shader->setColor(c);
#else
	shader->setColor(g.getColor());
#endif

	ScopedAlphaBlend b(g, true);

	if (scale != 1) {
		fonsDrawText(fs, 0, 0, text.c_str(), nullptr);
		g.popMatrix();
	} else {
		fonsDrawText(fs, x, y, text.c_str(), nullptr);
	}
}
void Font::addVerts(const std::string &text,
					glm::vec2 c,
					std::vector<glm::vec2> &verts,
					std::vector<glm::vec2> &uvs,
					HTextAlign halign,
					VTextAlign valign) {
	auto a = getRect(text, 0, 0);

	float x = c.x; // default left align
	float y = c.y - a.y; // default top align

	if (halign == HTextAlign::Centre) {
		x = c.x - a.width / 2.f;
	} else if (halign == HTextAlign::Right) {
		x = c.x - a.width;
	}

	if (valign == VTextAlign::Centre) {
		y = c.y - a.centre().y;
	} else if (valign == VTextAlign::Bottom) {
		y = c.y - a.bottom();
	}

	addVerts(text, vec2(x, y), verts, uvs);
}

void Font::addVerts(const std::string &text,
					glm::vec2 c,
					std::vector<glm::vec2> &verts,
					std::vector<glm::vec2> &uvs) {
	if (fs == nullptr) {
		Log::e() << "Calling getRect on null Font";
		return;
	}

	fonsAddVerts(fs, c.x, c.y, text.c_str(), nullptr, verts, uvs);
}

Font::VerticalMetrics Font::getVerticalMetrics() const {
	return verticalMetrics;
}
void Font::draw(Graphics &g, const std::string &text, glm::vec2 c, HTextAlign halign, VTextAlign valign) {
	auto a = getRect(text, 0, 0);

	float x = c.x; // default left align
	float y = c.y - a.y; // default top align

	if (halign == HTextAlign::Centre) {
		x = c.x - a.width / 2.f;
	} else if (halign == HTextAlign::Right) {
		x = c.x - a.width;
	}

	if (valign == VTextAlign::Centre) {
		y = c.y - a.centre().y;
	} else if (valign == VTextAlign::Bottom) {
		y = c.y - a.bottom();
	} else if (valign == VTextAlign::Baseline) {
		y = c.y;
	}

	draw(g, text, x, y);
}

void Font::drawVerticallyCentred(Graphics &g, const string &text, glm::vec2 c, HTextAlign align) {
	if (align == HTextAlign::Centre) {
		drawCentred(g, text, c);
	} else {
		auto a	= getRect(text, 0, 0);
		auto by = c.y - a.centre().y;

		if (align == HTextAlign::Left) {
			draw(g, text, c.x, by);
		} else { // TextAlign::Right
			draw(g, text, c.x - a.width, by);
		}
	}
}

void Font::drawHorizontallyCentred(Graphics &g, const string &text, glm::vec2 c) {
	auto a = getRect(text, 0, 0);
	auto b = c - a.centre();
	draw(g, text, b.x, c.y);
}

void Font::drawCentred(Graphics &g, const string &text, glm::vec2 c) {
	auto a = getRect(text, 0, 0);
	auto b = c - a.centre();
	draw(g, text, b.x, b.y);
}

// draws the string making c the bottom left corner
void Font::drawBottomLeftAligned(Graphics &g, const std::string &text, glm::vec2 c) {
	auto a = getRect(text, 0, 0);
	auto b = c - a.bl();
	draw(g, text, b.x, b.y);
}

Font::~Font() {
#ifdef __ANDROID__
	for (int i = 0; i < fonts.size(); i++) {
		if (fonts[i] == this) {
			fonts.erase(fonts.begin() + i);
			break;
		}
	}
#endif
	clear();
}

void Font::clear() {
#if !defined(MZGL_SOKOL_METAL)
	if (fs) {
		GLFONScontext *gl = (GLFONScontext *) (fs->params.userPtr);

		gl->TCOORD_ATTRIB = 0;
		gl->VERTEX_ATTRIB = 0;

		glfonsDelete(fs);
		fs = nullptr;
	}
#endif
}

Font::Font() {
#ifdef __ANDROID__
	fonts.push_back(this);
#endif
}

#include "stringUtil.h"

static std::pair<std::string, std::string> splitAtWidth(const Font &f, const std::string &word, float width) {
	std::string first  = "";
	std::string second = "";
	for (int i = word.size(); i > 0; i--) {
		auto sub = word.substr(0, i);
		if (f.getWidth(sub) <= width) {
			first  = sub;
			second = word.substr(i);
			break;
		}
	}
	return std::make_pair(first, second);
}

static bool isDelimiter(char c, const std::string &delimiters) {
	return delimiters.find(c) != std::string::npos;
}

// Tokenizer function that includes delimiters as separate tokens
static std::vector<std::string> tokenize(const std::string &source, const std::string &delimiters) {
	std::vector<std::string> tokens;
	std::string currentToken;

	for (char c: source) {
		if (isDelimiter(c, delimiters)) {
			if (!currentToken.empty()) {
				tokens.push_back(currentToken);
				currentToken.clear();
			}
			tokens.push_back(std::string(1, c)); // Push the delimiter as a separate token
		} else {
			currentToken += c;
		}
	}

	if (!currentToken.empty()) {
		tokens.push_back(currentToken); // Add the last token if it exists
	}

	return tokens;
}

static void addLines(const Font &f, std::vector<std::string> &lines, const std::string &para, float width) {
	if (para.empty()) lines.push_back("");

	auto words			 = tokenize(para, " \t,./-:");
	std::string currLine = "";
	for (auto it = words.begin(); it != words.end(); it++) {
		auto lineWithNextWord = currLine + (*it);
		if (f.getWidth(lineWithNextWord) > width) {
			if (f.getWidth((*it)) > width) {
				// this word is too long for the given paragraph width.
				// If the string contains one or more '/', work backwards
				// searching for '/' to find the longest string that will
				// fit on the line with a '/'
				// printf("Word too long: %s\n", (*it).c_str());
				// break the word down
				auto &word		 = *it;
				auto beforeAfter = splitAtWidth(f, lineWithNextWord, width);
				lines.push_back(beforeAfter.first);
				*it = beforeAfter.second;
				// printf("first: %s second: %s\n", beforeAfter.first.c_str(), beforeAfter.second.c_str());
				// it--;
				// continue;
			}

			if (!currLine.empty()) {
				lines.push_back(currLine);
			}
			it--;
			currLine = "";
		} else {
			currLine = lineWithNextWord;
			if (currLine == " ") currLine = "";
		}
	}
	if (!currLine.empty()) {
		lines.push_back(currLine);
	}
}
std::vector<std::string> Font::wrapText(const std::string &text, float width) const {
	std::vector<std::string> lines;
	auto paragraphs = split(text, "\n", false, true);

	for (auto &para: paragraphs) {
		addLines(*this, lines, para, width);
	}
	return lines;
}
