#pragma once

#include <string>
#include <vector>

class Base64 {
public:
	[[nodiscard]] static std::vector<char> decode(const std::string &base64);
	[[nodiscard]] static std::string encode(const std::vector<unsigned char> &data);
};
