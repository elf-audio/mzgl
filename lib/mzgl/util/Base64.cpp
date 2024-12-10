#include "Base64.h"
#include <stdexcept>

std::vector<char> Base64::decode(const std::string &base64) {
	static const std::string base64Chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

	auto findChar = [&](char c) -> int {
		auto pos = base64Chars.find(c);
		if (pos == std::string::npos) {
			throw std::invalid_argument("Invalid Base64 character");
		}
		return static_cast<int>(pos);
	};

	std::vector<char> decodedData;
	size_t length = base64.size();
	size_t i	  = 0;

	while (i < length) {
		uint32_t triple = 0;
		int padding		= 0;

		for (int j = 0; j < 4; ++j) {
			if (i >= length) {
				if (j < 2) {
					throw std::invalid_argument("Invalid Base64 string length");
				}
				triple <<= 6;
				++padding;
			} else if (base64[i] == '=') {
				triple <<= 6;
				++padding;
			} else {
				triple = (triple << 6) | findChar(base64[i]);
			}
			++i;
		}

		for (int j = 2; j >= 0; --j) {
			if (j >= padding) {
				decodedData.push_back(static_cast<char>((triple >> (j * 8)) & 0xFF));
			}
		}
	}

	return decodedData;
}

std::string Base64::encode(const std::vector<unsigned char> &data) {
	static const char base64_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
									   "abcdefghijklmnopqrstuvwxyz"
									   "0123456789+/";

	std::string encoded;
	size_t i = 0;
	unsigned char char_array_3[3];
	unsigned char char_array_4[4];

	for (unsigned char c: data) {
		char_array_3[i++] = c;
		if (i == 3) {
			char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
			char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
			char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
			char_array_4[3] = char_array_3[2] & 0x3f;

			for (i = 0; i < 4; i++) {
				encoded += base64_chars[char_array_4[i]];
			}
			i = 0;
		}
	}

	if (i > 0) {
		for (size_t j = i; j < 3; j++) {
			char_array_3[j] = '\0';
		}

		char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
		char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
		char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
		char_array_4[3] = char_array_3[2] & 0x3f;

		for (size_t j = 0; j < i + 1; j++) {
			encoded += base64_chars[char_array_4[j]];
		}

		while ((i++ < 3)) {
			encoded += '=';
		}
	}

	return encoded;
}