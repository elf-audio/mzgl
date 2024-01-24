//
//  Header.h
//  koala
//
//  Created by Marek Bereza on 13/08/2019.
//  Copyright © 2019 Marek Bereza. All rights reserved.
//

#pragma once

#include <nlohmann/json.hpp>
#include <string>
#include <glm/glm.hpp>

// these both throw std::runtime_error if they fail
void saveJson(const nlohmann::json &j, const std::string &path);
void loadJson(nlohmann::json &j, const std::string &path);

std::string getJsonString(nlohmann::json &j);

namespace glm {
	void to_json(nlohmann::json &j, const glm::vec2 &P);
	void from_json(const nlohmann::json &j, glm::vec2 &P);

	void to_json(nlohmann::json &j, const glm::vec3 &P);
	void from_json(const nlohmann::json &j, glm::vec3 &P);

	void to_json(nlohmann::json &j, const glm::vec4 &P);
	void from_json(const nlohmann::json &j, glm::vec4 &P);
} // namespace glm
