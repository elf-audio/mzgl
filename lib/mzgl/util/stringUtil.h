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


std::string to_string(float value, int precision);
std::string to_string(double value, int precision);

std::string toLowerCase(std::string s);
std::string toUpperCase(std::string s);

void replaceAll(std::string & data, std::string toSearch, std::string replaceStr);
std::string byteSizeToString(uint64_t bytes);

// from oF
std::vector <std::string> split(const std::string & source, const std::string & delimiter, bool ignoreEmpty = false, bool trim = false);

std::string trim(const std::string & src);
std::string trimFront(const std::string & src);
std::string trimBack(const std::string & src);

template <typename T>
std::string to_string(const T a_value, const int n);




