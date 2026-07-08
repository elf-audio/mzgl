#include "tests.h"
#include "Font.h"

// monospace measure: 1 unit per character, so widths in these tests are character counts
static float monoWidth(const std::string &s) {
	return static_cast<float>(s.size());
}

static std::vector<std::string> wrap(const std::string &text, float width) {
	return Font::wrapText(text, width, monoWidth);
}

static bool noLineStartsWith(const std::vector<std::string> &lines, const std::string &prefix) {
	for (const auto &line: lines) {
		if (line.rfind(prefix, 0) == 0) return false;
	}
	return true;
}

TEST_CASE("wrapText doesn't break between '.' and ')'", "[wrapText]") {
	// width chosen so the ")" on its own would overflow the first line -
	// previously this wrapped to a lone ")" on the second line
	auto lines = wrap("(3 other purchases were restored.)", 33);
	REQUIRE(lines.size() == 2);
	REQUIRE(lines[1] == "restored.)");
	REQUIRE(noLineStartsWith(lines, ")"));
}

TEST_CASE("wrapText doesn't start a line with '.'", "[wrapText]") {
	// width fits "were restored" but not the trailing "." -
	// previously this wrapped to a lone "." on the second line
	auto lines = wrap("were restored.", 13);
	REQUIRE(lines.size() == 2);
	REQUIRE(lines[1] == "restored.");
	REQUIRE(noLineStartsWith(lines, "."));
}

TEST_CASE("wrapText keeps runs of clinging punctuation together", "[wrapText]") {
	auto lines = wrap("samples (and packs, etc.), restored", 8);
	REQUIRE(noLineStartsWith(lines, ")"));
	REQUIRE(noLineStartsWith(lines, "."));
	REQUIRE(noLineStartsWith(lines, ","));
	// nothing lost in the merge
	std::string joined;
	for (const auto &line: lines) {
		joined += line;
	}
	REQUIRE(joined == "samples (and packs, etc.), restored");
}

TEST_CASE("wrapText clinging punctuation doesn't attach to whitespace", "[wrapText]") {
	auto lines = wrap("weird ) input", 100);
	REQUIRE(lines.size() == 1);
	REQUIRE(lines[0] == "weird ) input");
}

TEST_CASE("wrapText still breaks after slashes in paths", "[wrapText]") {
	auto lines = wrap("path/to/file", 8);
	REQUIRE(lines.size() == 2);
	REQUIRE(lines[0] == "path/to/");
	REQUIRE(lines[1] == "file");
}

TEST_CASE("wrapText basic behaviour unchanged", "[wrapText]") {
	SECTION("empty string gives one empty line") {
		REQUIRE(wrap("", 10) == std::vector<std::string> {""});
	}
	SECTION("text that fits stays on one line") {
		REQUIRE(wrap("hello there", 20) == std::vector<std::string> {"hello there"});
	}
	SECTION("newlines split paragraphs") {
		auto lines = wrap("one\ntwo", 20);
		REQUIRE(lines == std::vector<std::string> {"one", "two"});
	}
	SECTION("words longer than the width get split") {
		auto lines = wrap("aaaaaaaaaa", 4);
		REQUIRE(lines == std::vector<std::string> {"aaaa", "aaaa", "aa"});
	}
}
