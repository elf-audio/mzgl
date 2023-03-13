//
//  stringUtil.cpp
//  mzgl
//
//  Created by Marek Bereza on 04/08/2022.
//  Copyright © 2022 Marek Bereza. All rights reserved.
//

#include "stringUtil.h"

#include <sstream>
#include <iomanip>
#include <algorithm>

std::string to_string(float value, int precision){
	std::ostringstream out;
	out << std::fixed << std::setprecision(precision) << value;
	return out.str();
}

std::string to_string(double value, int precision){
	return to_string((float)value, precision);
}


template <typename T>
std::string to_string(const T a_value, const int n)
{
	std::ostringstream out;
	out << std::setprecision(n) << a_value;
	return out.str();
}


std::string byteSizeToString(uint64_t bytes) {

	char buf[256];
	double size = bytes;
	int i = 0;
	const char* units[] = {"B", "kB", "MB", "GB", "TB", "PB", "EB", "ZB", "YB"};
	while (size > 1024) {
		size /= 1024;
		i++;
	}
	snprintf(buf, sizeof(buf), "%.*f %s", i, size, units[i]);
	return buf;
}


void replaceAll(std::string & d, std::string toSearch, std::string replaceStr) {


	// Get the first occurrence
	size_t pos = d.find(toSearch);

	// Repeat till end is reached
	while( pos != std::string::npos) {
		// Replace this occurrence of Sub String
		d.replace(pos, toSearch.size(), replaceStr);
		// Get the next occurrence from the current position
		pos = d.find(toSearch, pos + replaceStr.size());
	}
}



std::string urlencode(const std::string& value) {
	static auto hex_digt = "0123456789ABCDEF";

	std::string result;
	result.reserve(value.size() << 1);

	for (auto ch : value) {
		if ((ch >= '0' && ch <= '9')
			|| (ch >= 'A' && ch <= 'Z')
			|| (ch >= 'a' && ch <= 'z')
			|| ch == '-' || ch == '_' || ch == '!'
			|| ch == '\'' || ch == '(' || ch == ')'
			|| ch == '*' || ch == '~' || ch == '.')  /*  !'()*-._~   */{
			result.push_back(ch);
		} else {
			result += std::string("%") +
					  hex_digt[static_cast<unsigned char>(ch) >> 4]
					  +  hex_digt[static_cast<unsigned char>(ch) & 15];
		}
	}

	return result;
}

std::string urldecode(const std::string& value) {
	std::string result;
	result.reserve(value.size());

	for (std::size_t i = 0; i < value.size(); ++i) {
		auto ch = value[i];

		if (ch == '%' && (i + 2) < value.size()) {
			auto hex = value.substr(i + 1, 2);
			auto dec = static_cast<char>(std::strtol(hex.c_str(), nullptr, 16));
			result.push_back(dec);
			i += 2;
		} else if (ch == '+') {
			result.push_back(' ');
		} else {
			result.push_back(ch);
		}
	}

	return result;
}




//--------------------------------------------------
std::vector <std::string> split(const std::string & source, const std::string & delimiter, bool ignoreEmpty, bool _trim) {
	std::vector<std::string> result;
	if (delimiter.empty()) {
		result.push_back(source);
		return result;
	}
	std::string::const_iterator substart = source.begin(), subend;
	while (true) {
		subend = search(substart, source.end(), delimiter.begin(), delimiter.end());
		std::string sub(substart, subend);
		if(_trim) {
			sub = trim(sub);
		}
		if (!ignoreEmpty || !sub.empty()) {
			result.push_back(sub);
		}
		if (subend == source.end()) {
			break;
		}
		substart = subend + delimiter.size();
	}
	return result;
}
//--------------------------------------------------
std::string trimFront(const std::string & src){
	auto dst = src;
	dst.erase(dst.begin(),std::find_if_not(dst.begin(),dst.end(),[&](char & c){return std::isspace(c);}));
	return dst;
}

//--------------------------------------------------
std::string trimBack(const std::string & src){
	auto dst = src;
	dst.erase(std::find_if_not(dst.rbegin(),dst.rend(),[&](char & c){return std::isspace(c);}).base(), dst.end());
	return dst;
}

//--------------------------------------------------
std::string trim(const std::string & src){
	return trimFront(trimBack(src));
}
