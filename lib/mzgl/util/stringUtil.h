//
//  stringUtil.h
//  mzgl
//
//  Created by Marek Bereza on 04/08/2022.
//  Copyright © 2022 Marek Bereza. All rights reserved.
//

#pragma once

#include <string>
#include <vector>
#include <chrono>

// to 'precision' decimal places
std::string to_string(float value, int precision);
std::string to_string(double value, int precision);

// to 'sigFigs' significant figures
std::string toSigFigs(float value, int sigFigs);

// 100.0494 -> 100.05, 100.0001 -> 100, 100.000 ->100
std::string toDecimalPlacesIfNeeded(float value, int decimalPlaces);

std::string toLowerCase(std::string s);
std::string toUpperCase(std::string s);

void replaceAll(std::string &data, std::string toSearch, std::string replaceStr);
std::string byteSizeToString(uint64_t bytes);

enum class CaseSensitivity { caseSensitive, caseInSensitive };
[[nodiscard]] bool
	startsWith(const std::string &stringToSearch, const std::string &prefix, CaseSensitivity caseSensitivity);
[[nodiscard]] bool
	endsWith(const std::string &stringToSearch, const std::string &suffix, CaseSensitivity caseSensitivity);

// from oF
std::vector<std::string>
	split(const std::string &source, const std::string &delimiter, bool ignoreEmpty = false, bool trim = false);

std::string trim(const std::string &src);
std::string trimFront(const std::string &src);
std::string trimBack(const std::string &src);

template <typename T>
std::string to_string(const T a_value, const int n);
std::string zeroPad(int number, int width);
std::string zeroPad2(int number);

std::string time_point_to_string(const std::chrono::system_clock::time_point &tp);
std::chrono::system_clock::time_point string_to_time_point(const std::string &s);
std::string timestampString();