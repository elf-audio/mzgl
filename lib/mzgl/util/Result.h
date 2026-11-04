
#pragma once

#include <vector>
#include <string>
#include "stringUtil.h"

// What happened when something was attempted.
//
// issues   - errors. Any issue means it didn't (fully) work: success() is false.
// warnings - it worked, but there's something the user should know (e.g. the
//            file was converted on the way in). Never affects success().
//
// Subclass it to return a value alongside (see the various LoadResults), so
// sub-results can be merged into a parent with +=.
class Result {
public:
	Result() = default;
	Result(const std::string &issue) { addIssue(issue); }
	std::vector<std::string> issues;
	std::vector<std::string> warnings;

	[[nodiscard]] bool success() const { return issues.empty(); }
	[[nodiscard]] bool hasWarnings() const { return !warnings.empty(); }

	void addIssue(const std::string &issue) { issues.push_back(issue); }
	void addWarning(const std::string &warning) { warnings.push_back(warning); }

	Result &operator+=(const Result &other) {
		issues.insert(issues.end(), other.issues.begin(), other.issues.end());
		warnings.insert(warnings.end(), other.warnings.begin(), other.warnings.end());
		return *this;
	}

	std::string getIssueList() const { return bulletList(issues); }
	std::string getWarningList() const { return bulletList(warnings); }

private:
	static std::string bulletList(const std::vector<std::string> &items) {
		if (items.empty()) return "";
		return "- " + join(items, "\n- ");
	}
};
