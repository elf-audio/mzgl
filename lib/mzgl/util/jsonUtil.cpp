//
//  Header.h
//  koala
//
//  Created by Marek Bereza on 13/08/2019.
//  Copyright © 2019 Marek Bereza. All rights reserved.
//

#include <mzgl/util/jsonUtil.h>
#include <fsystem/fsystem.h>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <mzgl/util/log.h>

void saveJson(const nlohmann::json &j, const std::string &pathToLoad) {
	std::ofstream jsonFile(fs::path {pathToLoad});

	if (!jsonFile.good()) {
		Log::e() << "ERROR SAVING JSON " << pathToLoad;
		throw std::runtime_error("saveJson: " + std::string(strerror(errno)));
	}

	jsonFile << std::setfill('\t') << std::setw(1) << j << std::endl;
	if (!jsonFile.good()) {
		throw std::runtime_error("saveJson: " + std::string(strerror(errno)));
	}
}

void loadJson(nlohmann::json &j, const std::string &path) {
	std::ifstream jsonFile(fs::u8path(path));
	// Log::d() << "LOADING JSON FROM " << path;
	if (!jsonFile.good()) {
		Log::e() << "ERROR LOADING JSON - " << path;
		throw std::runtime_error("loadJson: " + std::string(strerror(errno)));
	}
	try {
		jsonFile >> j;
	} catch (const nlohmann::detail::parse_error &err) {
		Log::e() << "Got json parse error for file " << path << ": " << err.what();
		throw std::runtime_error("loadJson - json parse error: " + std::string(strerror(errno)));
	}
}
std::string getJsonString(nlohmann::json &j) {
	std::stringstream str;

	str << std::setfill('\t') << std::setw(1) << j << std::endl;
	return str.str();
}
using namespace nlohmann;
namespace glm {
	void to_json(json &j, const glm::vec2 &P) {
		j = {{"x", P.x}, {"y", P.y}};
	};

	void from_json(const json &j, glm::vec2 &P) {
		P.x = j.at("x").get<double>();
		P.y = j.at("y").get<double>();
	}

	void to_json(json &j, const glm::vec3 &P) {
		j = {{"x", P.x}, {"y", P.y}, {"z", P.z}};
	};

	void from_json(const json &j, glm::vec3 &P) {
		P.x = j.at("x").get<double>();
		P.y = j.at("y").get<double>();
		P.z = j.at("z").get<double>();
	}

	void to_json(json &j, const glm::vec4 &P) {
		j = {{"x", P.x}, {"y", P.y}, {"z", P.z}, {"w", P.w}};
	};

	void from_json(const json &j, glm::vec4 &P) {
		P.x = j.at("x").get<double>();
		P.y = j.at("y").get<double>();
		P.z = j.at("z").get<double>();
		P.w = j.at("w").get<double>();
	}

} // namespace glm
