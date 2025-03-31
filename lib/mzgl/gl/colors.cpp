//
// Created by Marek Bereza on 28/03/2025.
//

#include "colors.h"
#include "log.h"
#include <sstream>
#include <array>
#include <algorithm>

glm::vec4 hexColor(int hex, float a) {
	glm::vec4 c;
	c.r = ((hex >> 16) & 0xFF) / 255.f;
	c.g = ((hex >> 8) & 0xFF) / 255.f;
	c.b = ((hex) & 0xFF) / 255.f;
	c.a = a;
	return c;
}

glm::vec3 rgb2hsv(glm::vec4 rgb) {
	float r = rgb.x;
	float g = rgb.y;
	float b = rgb.z;

	float maxVal = std::max({r, g, b});
	float minVal = std::min({r, g, b});
	float delta	 = maxVal - minVal;

	float h, s, v;
	v = maxVal;

	if (delta == 0) {
		h = 0;
		s = 0;
	} else {
		s = delta / maxVal;

		if (r == maxVal) {
			h = (g - b) / delta;
		} else if (g == maxVal) {
			h = 2 + (b - r) / delta;
		} else {
			h = 4 + (r - g) / delta;
		}

		h *= 60;
		if (h < 0) h += 360;
	}

	h = h / 360.0f; // Normalize to [0, 1]
	return {h, s, v};
}

glm::vec4 hsv2rgb(glm::vec3 hsv) {
	float h = hsv.x * 360.0f; // Convert back to 360 range
	float s = hsv.y;
	float v = hsv.z;

	int i	= int(std::floor(h / 60.0)) % 6;
	float f = h / 60.f - i;
	float p = v * (1 - s);
	float q = v * (1 - f * s);
	float t = v * (1 - (1 - f) * s);

	float r = 0.f, g = 0.f, b = 0.f;

	switch (i) {
		case 0:
			r = v;
			g = t;
			b = p;
			break;
		case 1:
			r = q;
			g = v;
			b = p;
			break;
		case 2:
			r = p;
			g = v;
			b = t;
			break;
		case 3:
			r = p;
			g = q;
			b = v;
			break;
		case 4:
			r = t;
			g = p;
			b = v;
			break;
		case 5:
			r = v;
			g = p;
			b = q;
			break;
	}

	return {r, g, b, 1.f};
}

int hexCharToInt(char c) {
	if (c >= '0' && c <= '9') return c - '0';
	if (c >= 'a' && c <= 'f') return 10 + c - 'a';
	if (c >= 'A' && c <= 'F') return 10 + c - 'A';
	return c;
}

glm::vec4 hexColor(std::string inp) {
	glm::vec4 a;

	// work out the format first
	if (inp.size() > 1 && inp[0] == '#') {
		inp = inp.substr(1);
	} else if (inp.size() > 2 && inp[0] == '0' && inp[1] == 'x') {
		inp = inp.substr(2);
	}
	if (inp.size() == 3) {
		inp = std::string(2, inp[0]) + std::string(2, inp[1]) + std::string(2, inp[2]);
	}

	a.x = (hexCharToInt(inp[0]) * 16 + hexCharToInt(inp[1])) / 255.f;
	a.y = (hexCharToInt(inp[2]) * 16 + hexCharToInt(inp[3])) / 255.f;
	a.z = (hexCharToInt(inp[4]) * 16 + hexCharToInt(inp[5])) / 255.f;
	a.w = 1.f;
	return a;
}

glm::vec4 svgHexColor(std::string hex) {
	return hexColor(static_cast<int>(strtol(hex.substr(1).c_str(), nullptr, 16)));
}

glm::vec4 rgbColor(std::string colourString) {
	auto start = colourString.find("rgb(");
	auto end   = colourString.find(")");

	if (start == std::string::npos && end == std::string::npos || start >= end) {
		return {};
	}

	std::string numbers = colourString.substr(start + 4, end - (start + 4));
	std::istringstream iss(numbers);
	std::string token;
	int i = 0;
	std::array<float, 3> colors {1.f, 1.f, 1.f};
	while (std::getline(iss, token, ',') && i < 3) {
		colors[i++] = static_cast<float>(std::clamp(std::stoi(token), 0, 255)) / 255.0f;
	}
	return glm::vec4 {colors[0], colors[1], colors[2], 1.f};
}

glm::vec4 rgbaColor(std::string colourString) {
	auto start = colourString.find("rgba(");
	auto end   = colourString.find(")");

	if (start == std::string::npos && end == std::string::npos || start >= end) {
		Log::e() << "Bad RGBA string format (" << colourString << ")";
		return {};
	}

	std::string numbers = colourString.substr(start + 5, end - (start + 5));
	std::istringstream iss(numbers);
	std::string token;
	int i = 0;
	std::array<float, 4> colours {1.f, 1.f, 1.f, 1.f};
	while (std::getline(iss, token, ',') && i < 3) {
		colours[i++] = static_cast<float>(std::clamp(std::stoi(token), 0, 255)) / 255.0f;
	}

	std::getline(iss, token, ',');
	colours[i++] = static_cast<float>(std::clamp(std::stof(token), 0.f, 1.f));

	if (i != 4) {
		Log::e() << "RGB Colour didn't have enough elements " << colourString;
	}

	return glm::vec4 {colours[0], colours[1], colours[2], colours[3]};
}

glm::vec4 namedColor(std::string name) {
	if (name == "none" || name == "transparent") return {0, 0, 0, 0};
	if (name == "black") return hexColor(0);
	if (name == "silver") return hexColor(0xC0C0C0);
	if (name == "gray") return hexColor(0x808080);
	if (name == "white") return hexColor(0xFFFFFF);
	if (name == "maroon") return hexColor(0x800000);
	if (name == "red") return hexColor(0xFF0000);
	if (name == "purple") return hexColor(0x800080);
	if (name == "fuchsia") return hexColor(0xFF00FF);
	if (name == "green") return hexColor(0x008000);
	if (name == "lime") return hexColor(0x00FF00);
	if (name == "olive") return hexColor(0x808000);
	if (name == "yellow") return hexColor(0xFFFF00);
	if (name == "navy") return hexColor(0x000080);
	if (name == "blue") return hexColor(0x0000FF);
	if (name == "teal") return hexColor(0x008080);
	if (name == "aqua") return hexColor(0x00FFFF);

	Log::e() << "Didnt find colour named " << name;
	return hexColor(0xFFFFFF);
}

bool isHexColor(std::string s) {
	if (s.empty() || s[0] != '#' || (s.size() != 4 && s.size() != 7)) {
		return false;
	}

	return std::all_of(s.begin() + 1, s.end(), [](char c) { return std::isxdigit(c); });
}

bool isRGBColor(std::string s) {
	if (s.size() < 10 || s.find("rgb(") != 0 || s.back() != ')') {
		return false;
	}

	return std::all_of(
		s.begin() + 4, s.end() - 1, [](char c) { return std::isdigit(c) || c == ',' || std::isspace(c); });
}

bool isRGBAColor(std::string s) {
	if (s.size() < 15 || s.find("rgba(") != 0 || s.back() != ')') {
		return false;
	}

	return std::all_of(s.begin() + 5, s.end() - 1, [](char c) {
		return std::isdigit(c) || c == ',' || c == '.' || std::isspace(c);
	});
}
