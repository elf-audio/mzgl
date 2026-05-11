
#pragma once

#include <vector>
#include <string>
#include "stringUtil.h"
class Result {
public:
	Result() = default;
	Result(const std::string &issue) { addIssue(issue); }
	std::vector<std::string> issues;
	[[nodiscard]] bool success() const { return issues.empty(); }
	void addIssue(const std::string &issue) { issues.push_back(issue); }
	Result &operator+=(const Result &other) {
		issues.insert(issues.end(), other.issues.begin(), other.issues.end());
		return *this;
	}

	std::string getIssueList() const {
		if (issues.empty()) return "";
		return "- " + join(issues, "\n- ");
	}
};