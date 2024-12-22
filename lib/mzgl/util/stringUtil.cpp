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
#include <cmath>

std::string zeroPad2(int number) {
	return zeroPad(number, 2);
}

std::string zeroPad(int number, int width) {
	std::ostringstream oss;
	oss << std::setw(width) << std::setfill('0') << number;
	return oss.str();
}

std::string to_string(float value, int precision) {
	std::ostringstream out;
	out << std::fixed << std::setprecision(precision) << value;
	return out.str();
}


std::string timestampString() {
	auto now = std::time(nullptr);
	char buffer[20];
	std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", std::localtime(&now));
	return std::string(buffer);
}

// this implementation was occasionally using scientific notation
// and couldn't turn it off using std::fixed because then std::setprecision
// was setting decimal places and not significant figures.
//std::string toSigFigs(float value, int sigFigs){
//	std::ostringstream out;
//	out << std::setprecision(sigFigs) << value;
//	return out.str();
//}

// std::string toSigFigs(float f, int sigfig) {
// 	int d = ceil(log10(f < 0 ? -f : f));
// 	float scale = pow(10, d - sigfig);
// 	double ff = round(f / scale) * scale;

// 	// 	Convert to string
// 	std::string str = std::to_string(ff);

// 	// Truncate unnecessary trailing zeroes or decimal point
// 	str.erase(str.find_last_not_of('0') + 1, std::string::npos);
// 	if (str.back() == '.') {
// 		str.pop_back();
// 	}
// 	// printf("%f => %d scl: %f  %f       -> %s\n", f, d, scale, ff, str.c_str());
// 	return str;
// }

static bool isPowerOfTen(int n) {
	if (n < 1) {
		return false; // power of 10 is always positive
	}

	while (n % 10 == 0) {
		n /= 10; // dividing by 10
	}

	return n == 1; // a number is a power of 10 if it's 1 after the above process
}
/*
std::string toSigFigs(float f, int sigfig) {
	int d = ceil(log10(f < 0 ? -f : f));
	int pt = d - sigfig;
	float scale = pow(10, pt);
	int ff = round(f / scale);
	if (isPowerOfTen(ff)) {
		pt++;
		scale *= 10;
		ff = round(f / scale);
	}
	// 	Convert to string
	std::string str = std::to_string(ff);

	if (pt == 0) {
		// do nothing
	} else if (pt > 0) {
		printf("A\n");
		for (int i = 0; i < pt; i++) {
			str += "0";
		}
		// return str;
	} else if (-pt < str.length()) {
		printf("B\n");
		str.insert(str.length() + pt, ".");
	} else {
		printf("C\n");
		for (int i = 0; i < -1 - pt; i++) {
			str = "0" + str;
		}
		str.insert(1, ".");
	}

	return str;
}*/

std::string toSigFigs(float value, int sigFig) {
	// Take care of zero case
	if (value == 0.0) {
		return "0";
	}

	// Compute absolute value
	float absolute = std::abs(value);

	// Compute the number of digits before decimal point
	int digits = std::log10(absolute) + 1;

	// Compute the number of decimal places needed
	int decimalPlaces = std::max(sigFig - digits, 0);

	// Build format string
	std::string format = "%." + std::to_string(decimalPlaces) + "f";

	// Create buffer and perform formatting
	char buffer[50];
	std::snprintf(buffer, sizeof(buffer), format.c_str(), value);
	//	printf("%f(%d):  %s\n", value, sigFig, buffer);
	return std::string(buffer);
}

std::string to_string(double value, int precision) {
	return to_string((float) value, precision);
}

template <typename T>
std::string to_string(const T a_value, const int n) {
	std::ostringstream out;
	out << std::setprecision(n) << a_value;
	return out.str();
}

std::string toLowerCase(std::string s) {
	std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
	return s;
}

std::string toUpperCase(std::string s) {
	std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::toupper(c); });
	return s;
}

std::string byteSizeToString(uint64_t bytes) {
	char buf[256];
	double size			= bytes;
	int i				= 0;
	const char *units[] = {"B", "kB", "MB", "GB", "TB", "PB", "EB", "ZB", "YB"};
	while (size > 1024) {
		size /= 1024;
		i++;
	}
	snprintf(buf, sizeof(buf), "%.*f %s", i, size, units[i]);
	return buf;
}

void replaceAll(std::string &d, std::string toSearch, std::string replaceStr) {
	// Get the first occurrence
	size_t pos = d.find(toSearch);

	// Repeat till end is reached
	while (pos != std::string::npos) {
		// Replace this occurrence of Sub String
		d.replace(pos, toSearch.size(), replaceStr);
		// Get the next occurrence from the current position
		pos = d.find(toSearch, pos + replaceStr.size());
	}
}

//--------------------------------------------------
std::vector<std::string>
	split(const std::string &source, const std::string &delimiter, bool ignoreEmpty, bool _trim) {
	std::vector<std::string> result;
	if (delimiter.empty()) {
		result.push_back(source);
		return result;
	}
	std::string::const_iterator substart = source.begin(), subend;
	while (true) {
		subend = search(substart, source.end(), delimiter.begin(), delimiter.end());
		std::string sub(substart, subend);
		if (_trim) {
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
std::string trimFront(const std::string &src) {
	auto dst = src;
	dst.erase(dst.begin(), std::find_if_not(dst.begin(), dst.end(), [&](char &c) { return std::isspace(c); }));
	return dst;
}

//--------------------------------------------------
std::string trimBack(const std::string &src) {
	auto dst = src;
	dst.erase(std::find_if_not(dst.rbegin(), dst.rend(), [&](char &c) { return std::isspace(c); }).base(),
			  dst.end());
	return dst;
}

//--------------------------------------------------
std::string trim(const std::string &src) {
	return trimFront(trimBack(src));
}

std::string time_point_to_string(const std::chrono::system_clock::time_point &tp) {
	// Convert time_point to time_t
	std::time_t time_t_format = std::chrono::system_clock::to_time_t(tp);

	// Convert time_t to tm for local time
	std::tm tm_format = *std::localtime(&time_t_format);

	// Create a string stream to format the date and time
	std::ostringstream oss;
	oss << std::put_time(&tm_format, "%Y-%m-%d %H:%M:%S");

	return oss.str();
}

std::chrono::system_clock::time_point string_to_time_point(const std::string &s) {
	std::tm tm = {};
	std::istringstream ss(s);
	ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
	return std::chrono::system_clock::from_time_t(std::mktime(&tm));
}
