#pragma once

#include <string>
#include "glm/glm.hpp"

bool isHexColor(std::string s);
bool isRGBColor(std::string s);
bool isRGBAColor(std::string s);

glm::vec4 hexColor(int hex, float a = 1);
glm::vec4 hexColor(std::string s);

glm::vec4 svgHexColor(std::string s);
glm::vec4 namedColor(std::string s);

glm::vec4 rgbColor(std::string s);
glm::vec4 rgbaColor(std::string s);

glm::vec3 rgb2hsv(glm::vec4 rgb);
glm::vec4 hsv2rgb(glm::vec3 hsv);